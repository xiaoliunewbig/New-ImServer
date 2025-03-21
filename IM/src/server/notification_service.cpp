#include "server/notification_service.h"
#include "server/utils/logger.h"
#include "server/utils/security.h"
#include "server/utils/datetime.h"
#include "server/utils/config.h"
#include "server/utils/string_util.h"
#include "server/utils/jwt_verifier.h"
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>
#include <chrono>

namespace im {

using json = nlohmann::json;

// 添加NotificationDoneCallback类的正确定义
class NotificationServiceImpl::NotificationDoneCallback {
public:
    NotificationDoneCallback(NotificationServiceImpl* service, int64_t user_id, 
                    grpc::ServerWriter<Message>* writer, grpc::ServerContext* ctx)
            : service_(service), user_id_(user_id), writer_(writer), context_(ctx) {}
    
    ~NotificationDoneCallback() = default;
    
    // 静态回调方法，符合gRPC要求
    static void Done(void* tag, bool ok) {
        // 接管内存所有权并处理
        std::unique_ptr<NotificationDoneCallback> callback(
            static_cast<NotificationDoneCallback*>(tag));
        callback->HandleDone(ok);
    }
    
    void HandleDone(bool ok) {
        // 检查上下文是否已被取消
        const bool cancelled = !ok || context_->IsCancelled();
        if (cancelled) {
            LOG_INFO("通知流被取消，用户: {}", user_id_);
        } else {
            LOG_INFO("通知流正常完成，用户: {}", user_id_);
        }
        
        // 清理流
        service_->RemoveActiveStream(user_id_, writer_);
    }
    
private:
    NotificationServiceImpl* service_;  // 不拥有
    int64_t user_id_;
    grpc::ServerWriter<Message>* writer_;  // 不拥有
    grpc::ServerContext* context_;  // 不拥有
};

NotificationServiceImpl::NotificationServiceImpl(
    std::shared_ptr<db::MySQLConnection> mysql_conn,
    std::shared_ptr<db::RedisClient> redis_client,
    std::shared_ptr<kafka::KafkaProducer> kafka_producer
) : mysql_conn_(mysql_conn),
    redis_client_(redis_client),
    kafka_producer_(kafka_producer),
    message_service_(nullptr),
    user_service_(nullptr),
    running_(true) {
    
    // 启动Kafka消费线程
    kafka_consumer_thread_ = std::thread(&NotificationServiceImpl::KafkaConsumerThread, this);
    
    LOG_INFO("NotificationServiceImpl initialized");
}

NotificationServiceImpl::~NotificationServiceImpl() {
    running_ = false;
    if (kafka_consumer_thread_.joinable()) {
        kafka_consumer_thread_.join();
    }
}

grpc::Status NotificationServiceImpl::SubscribeNotifications(
        grpc::ServerContext* context, 
        const SubscriptionRequest* request,
        grpc::ServerWriter<Message>* writer) {
    // 验证令牌
    int64_t current_user_id;
    if (!ValidateToken(context, current_user_id)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "身份验证失败");
    }
    
    // 验证用户身份
    if (request->user_id() != current_user_id) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "无权订阅其他用户的通知");
    }
    
    // 注册通知流
    AddActiveStream(current_user_id, writer);
    
    // 设置完成事件回调
    auto callback = new NotificationDoneCallback(this, current_user_id, writer, context);
    context->AsyncNotifyWhenDone(callback);
    
    // 发送欢迎消息
    Message welcome_msg;
    welcome_msg.set_message_id(0);
    welcome_msg.set_from_user_id(0);  // 系统消息
    welcome_msg.set_to_user_id(current_user_id);
    welcome_msg.set_message_type(im::MessageType::TEXT);
    welcome_msg.set_content("欢迎回来！您已成功连接到通知服务。");
    welcome_msg.set_send_time(utils::DateTime::NowSeconds());
    welcome_msg.set_is_read(false);
    welcome_msg.set_extra_info("{\"type\": \"system_notification\", \"category\": \"welcome\"}");
    
    if (!writer->Write(welcome_msg)) {
        RemoveActiveStream(current_user_id, writer);
        return grpc::Status(grpc::StatusCode::INTERNAL, "写入欢迎消息失败");
    }
    
    // 查询未读通知
    try {
        std::string sql = "SELECT id, type, content, create_time, is_read "
                         "FROM notifications "
                         "WHERE user_id = ? AND is_read = 0 "
                         "ORDER BY create_time DESC LIMIT 10";
        
        db::MySQLConnection::ResultSet results = mysql_conn_->ExecuteQuery(sql, {std::to_string(current_user_id)});
        
        for (const auto& notification : results) {
            Message notify_msg;
            // MySQL结果是std::map<std::string, std::string>，直接使用字符串
            std::string id_str = notification.at("id");
            notify_msg.set_message_id(std::stoll(id_str));
            notify_msg.set_from_user_id(0);  // 系统消息
            notify_msg.set_to_user_id(current_user_id);
            notify_msg.set_message_type(im::MessageType::TEXT);
            notify_msg.set_content(notification.at("content"));
            
            std::string create_time_str = notification.at("create_time");
            notify_msg.set_send_time(std::stoll(create_time_str));
            notify_msg.set_is_read(false);
            
            // 构造额外信息
            json extra;
            extra["type"] = "system_notification";
            extra["category"] = notification.at("type");
            extra["notification_id"] = notification.at("id");
            notify_msg.set_extra_info(extra.dump());
            
            if (!writer->Write(notify_msg)) {
                RemoveActiveStream(current_user_id, writer);
                return grpc::Status(grpc::StatusCode::INTERNAL, "写入未读通知失败");
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("获取未读通知失败: {}", e.what());
    }
    
    // 从Redis获取通知
    try {
        std::string notification_key = "user:" + std::to_string(current_user_id) + ":notifications";
        std::vector<std::string> notifications = redis_client_->ListRange(notification_key, 0, 9);
        
        for (const auto& notification_json : notifications) {
            try {
                // 注意：这里的notification是json对象，与MySQL结果不同
                json notification = json::parse(notification_json);
                
                Message notify_msg;
                // 根据不同类型进行处理
                int64_t msg_id = 0;
                
                if (notification.contains("id")) {
                    if (notification["id"].is_string()) {
                        msg_id = std::stoll(notification["id"].get<std::string>());
                    } else if (notification["id"].is_number()) {
                        msg_id = notification["id"].get<int64_t>();
                    }
                }
                
                notify_msg.set_message_id(msg_id);
                notify_msg.set_from_user_id(0);  // 系统消息
                notify_msg.set_to_user_id(current_user_id);
                notify_msg.set_message_type(im::MessageType::TEXT);
                
                std::string type = "general";
                if (notification.contains("type") && notification["type"].is_string()) {
                    type = notification["type"].get<std::string>();
                }
                
                if (type == "friend_request") {
                    int64_t from_user_id = 0;
                    std::string message_str;
                    
                    if (notification.contains("from_user_id")) {
                        if (notification["from_user_id"].is_number()) {
                            from_user_id = notification["from_user_id"].get<int64_t>();
                        } else if (notification["from_user_id"].is_string()) {
                            from_user_id = std::stoll(notification["from_user_id"].get<std::string>());
                        }
                    }
                    
                    if (notification.contains("message") && notification["message"].is_string()) {
                        message_str = notification["message"].get<std::string>();
                    }
                    
                    notify_msg.set_content("用户 " + std::to_string(from_user_id) + 
                                         " 发送了好友请求：" + message_str);
                    
                    // 设置额外信息
                    json extra;
                    extra["type"] = "friend_request";
                    extra["from_user_id"] = from_user_id;
                    
                    if (notification.contains("request_id")) {
                        if (notification["request_id"].is_string()) {
                            extra["request_id"] = notification["request_id"].get<std::string>();
                        } else if (notification["request_id"].is_number()) {
                            extra["request_id"] = notification["request_id"].get<int64_t>();
                        }
                    }
                    
                    notify_msg.set_extra_info(extra.dump());
                } 
                else if (type == "file_transfer_request") {
                    int64_t from_user_id = 0;
                    std::string file_name;
                    
                    if (notification.contains("from_user_id")) {
                        if (notification["from_user_id"].is_number()) {
                            from_user_id = notification["from_user_id"].get<int64_t>();
                        } else if (notification["from_user_id"].is_string()) {
                            from_user_id = std::stoll(notification["from_user_id"].get<std::string>());
                        }
                    }
                    
                    if (notification.contains("file_name") && notification["file_name"].is_string()) {
                        file_name = notification["file_name"].get<std::string>();
                    }
                    
                    notify_msg.set_content("用户 " + std::to_string(from_user_id) + 
                                         " 想要发送文件：" + file_name);
                    
                    // 设置额外信息
                    json extra;
                    extra["type"] = "file_transfer_request";
                    extra["from_user_id"] = from_user_id;
                    
                    if (notification.contains("request_id")) {
                        if (notification["request_id"].is_string()) {
                            extra["request_id"] = notification["request_id"].get<std::string>();
                        } else if (notification["request_id"].is_number()) {
                            extra["request_id"] = notification["request_id"].get<int64_t>();
                        }
                    }
                    
                    extra["file_name"] = file_name;
                    
                    if (notification.contains("file_size")) {
                        if (notification["file_size"].is_number()) {
                            extra["file_size"] = notification["file_size"].get<int64_t>();
                        } else if (notification["file_size"].is_string()) {
                            extra["file_size"] = notification["file_size"].get<std::string>();
                        }
                    }
                    
                    notify_msg.set_extra_info(extra.dump());
                } 
                else {
                    // 其他类型通知
                    if (notification.contains("content") && notification["content"].is_string()) {
                        notify_msg.set_content(notification["content"].get<std::string>());
                    } else {
                        notify_msg.set_content("您有一条新通知");
                    }
                    notify_msg.set_extra_info(notification_json);
                }
                
                // 处理时间戳
                int64_t timestamp = utils::DateTime::NowSeconds();
                if (notification.contains("timestamp")) {
                    if (notification["timestamp"].is_number()) {
                        timestamp = notification["timestamp"].get<int64_t>();
                    } else if (notification["timestamp"].is_string()) {
                        timestamp = std::stoll(notification["timestamp"].get<std::string>());
                    }
                }
                
                notify_msg.set_send_time(timestamp);
                notify_msg.set_is_read(false);
                
                if (!writer->Write(notify_msg)) {
                    RemoveActiveStream(current_user_id, writer);
                    return grpc::Status(grpc::StatusCode::INTERNAL, "写入Redis通知失败");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("解析通知JSON失败: {}", e.what());
                continue;
            }
        }
        
        // 标记通知为已读
        if (!notifications.empty()) {
            redis_client_->ListTrim(notification_key, notifications.size(), -1);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("获取Redis通知失败: {}", e.what());
    }
    
    // 设置用户在线状态
    std::string online_key = "user:" + std::to_string(current_user_id) + ":online";
    redis_client_->SetValue(online_key, "1", 3600);  // 1小时过期
    
    // 阻塞直到客户端断开连接
    while (!context->IsCancelled()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 定期更新在线状态
        redis_client_->SetValue(online_key, "1", 3600);
    }
    
    return grpc::Status::OK;
}

void NotificationServiceImpl::SetMessageService(MessageServiceImpl* message_service) {
    message_service_ = message_service;
}

void NotificationServiceImpl::SetUserService(UserServiceImpl* user_service) {
    user_service_ = user_service;
}

void NotificationServiceImpl::SendNotification(int64_t user_id, const Message& notification) {
    // 检查用户是否有活跃的通知流
    bool has_active_stream = false;
    {
        std::lock_guard<std::mutex> check_lock(streams_mutex_);
        auto it = active_streams_.find(user_id);
        has_active_stream = (it != active_streams_.end() && !it->second.empty());
    }
    
    // 如果用户有活跃的通知流，直接发送
    if (has_active_stream) {
        std::lock_guard<std::mutex> send_lock(streams_mutex_);
        auto it = active_streams_.find(user_id);
        if (it != active_streams_.end()) {
            // 向所有活跃的流发送通知
            for (auto writer : it->second) {
                if (!writer->Write(notification)) {
                    LOG_ERROR("发送通知到用户 {} 的流失败", user_id);
                }
            }
        }
    } else {
        // 如果用户没有活跃的通知流，存储到数据库和Redis
        try {
            // 存储到数据库
            std::string sql = "INSERT INTO notifications (user_id, type, content, create_time, is_read) "
                             "VALUES (?, ?, ?, ?, 0)";
            
            int64_t now = utils::DateTime::NowSeconds();
            std::string type = "general";
            
            try {
                json extra = json::parse(notification.extra_info());
                if (extra.contains("category") && extra["category"].is_string()) {
                    type = extra["category"].get<std::string>();
                }
            } catch (const std::exception& e) {
                LOG_ERROR("解析通知类型失败: {}", e.what());
            }
            
            mysql_conn_->ExecuteInsert(sql, {
                std::to_string(user_id),
                type,
                notification.content(),
                std::to_string(now)
            });
            
            // 存储到Redis
            std::string notification_key = "user:" + std::to_string(user_id) + ":notifications";
            
            json redis_notification;
            redis_notification["content"] = notification.content();
            redis_notification["type"] = "system_notification";
            redis_notification["timestamp"] = std::to_string(now);
            redis_notification["id"] = std::to_string(0); // 临时ID
            
            try {
                json extra = json::parse(notification.extra_info());
                for (auto& [key, value] : extra.items()) {
                    redis_notification[key] = value;
                }
            } catch (const std::exception& e) {
                LOG_ERROR("复制通知额外信息失败: {}", e.what());
            }
            
            redis_client_->ListPush(notification_key, redis_notification.dump());
            redis_client_->Expire(notification_key, 604800);  // 一周过期
        } catch (const std::exception& e) {
            LOG_ERROR("存储通知失败: {}", e.what());
        }
    }
}

void NotificationServiceImpl::BroadcastNotification(const Message& notification) {
    // 向所有活跃的用户发送通知
    std::lock_guard<std::mutex> lock(streams_mutex_);
    for (auto& user_streams : active_streams_) {
        for (auto writer : user_streams.second) {
            if (!writer->Write(notification)) {
                LOG_ERROR("广播通知到用户 {} 的流失败", user_streams.first);
            }
        }
    }
    
    // 将系统公告存储到数据库，以便离线用户登录时查看
    try {
        std::string sql = "INSERT INTO system_announcements (title, content, sender_id, create_time) "
                        "VALUES (?, ?, ?, ?)";
        
        int64_t now = utils::DateTime::NowSeconds();
        std::string title = "系统通知";
        
        try {
            json extra = json::parse(notification.extra_info());
            if (extra.contains("title") && extra["title"].is_string()) {
                title = extra["title"].get<std::string>();
            }
        } catch (const std::exception& e) {
            LOG_ERROR("解析通知额外信息失败: {}", e.what());
        }
        
        mysql_conn_->ExecuteInsert(sql, {
            title,
            notification.content(),
            std::to_string(notification.from_user_id()),
            std::to_string(now)
        });
    } catch (const std::exception& e) {
        LOG_ERROR("存储系统公告失败: {}", e.what());
    }
}

bool NotificationServiceImpl::ValidateToken(grpc::ServerContext* context, int64_t& user_id) {
    try {
        std::string token = GetAuthToken(context);
        if (token.empty()) {
            return false;
        }
        
        // 解析JWT令牌
        json payload = jwt::JWTVerifier::Verify(token);
        
        // 获取用户ID
        if (payload.contains("user_id")) {
            if (payload["user_id"].is_number()) {
                user_id = payload["user_id"].get<int64_t>();
            } else if (payload["user_id"].is_string()) {
                user_id = std::stoll(payload["user_id"].get<std::string>());
            } else {
                LOG_ERROR("令牌中的user_id格式无效");
                return false;
            }
            return true;
        } else {
            LOG_ERROR("令牌中缺少user_id字段");
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("验证令牌失败: {}", e.what());
        return false;
    }
}

std::string NotificationServiceImpl::GetAuthToken(grpc::ServerContext* context) {
    auto metadata = context->client_metadata();
    auto token_iter = metadata.find("authorization");
    if (token_iter != metadata.end()) {
        std::string auth_header(token_iter->second.data(), token_iter->second.length());
        if (auth_header.substr(0, 7) == "Bearer ") {
            return auth_header.substr(7);
        }
    }
    return "";
}

bool NotificationServiceImpl::IsUserOnline(int64_t user_id) const {
    // 简化实现，直接检查Redis键是否存在
    // 不使用streams_mutex_，避免const方法中使用非const mutex的问题
    std::string online_key = "user:" + std::to_string(user_id) + ":online";
    return redis_client_->KeyExists(online_key);
}

void NotificationServiceImpl::KafkaConsumerThread() {
    // TODO: 实现Kafka消费逻辑
    // 此处需要使用Kafka消费API订阅im_events主题
    // 并处理各种事件通知
    
    LOG_INFO("Kafka消费线程启动");
    
    while (running_) {
        try {
            // 模拟Kafka消费逻辑
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 在实际应用中，这里应该是从Kafka接收消息
            // 并调用HandleKafkaEvent处理
        } catch (const std::exception& e) {
            LOG_ERROR("Kafka消费线程异常: {}", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    LOG_INFO("Kafka消费线程退出");
}

void NotificationServiceImpl::HandleKafkaEvent(const std::string& event_json) {
    try {
        json event = json::parse(event_json);
        std::string event_type;
        
        if (event.contains("event_type") && event["event_type"].is_string()) {
            event_type = event["event_type"].get<std::string>();
        } else {
            LOG_ERROR("无效的事件类型");
            return;
        }
        
        if (event_type == "message_sent") {
            // 处理新消息事件
            int64_t to_user_id = 0;
            
            if (event.contains("to_user_id")) {
                if (event["to_user_id"].is_number()) {
                    to_user_id = event["to_user_id"].get<int64_t>();
                } else if (event["to_user_id"].is_string()) {
                    to_user_id = std::stoll(event["to_user_id"].get<std::string>());
                }
            }
            
            // 如果用户在线且有活跃的通知流，直接推送
            if (IsUserOnline(to_user_id)) {
                if (message_service_) {
                    // 转发给消息服务处理
                    // 在实际应用中，应该从事件中提取完整的消息对象
                    Message message;
                    int64_t message_id = 0;
                    int64_t from_user_id = 0;
                    int message_type = 0;
                    int64_t timestamp = utils::DateTime::NowSeconds();
                    
                    // 安全地提取各个字段
                    if (event.contains("message_id")) {
                        if (event["message_id"].is_number()) {
                            message_id = event["message_id"].get<int64_t>();
                        } else if (event["message_id"].is_string()) {
                            message_id = std::stoll(event["message_id"].get<std::string>());
                        }
                    }
                    
                    if (event.contains("from_user_id")) {
                        if (event["from_user_id"].is_number()) {
                            from_user_id = event["from_user_id"].get<int64_t>();
                        } else if (event["from_user_id"].is_string()) {
                            from_user_id = std::stoll(event["from_user_id"].get<std::string>());
                        }
                    }
                    
                    if (event.contains("message_type")) {
                        if (event["message_type"].is_number()) {
                            message_type = event["message_type"].get<int>();
                        } else if (event["message_type"].is_string()) {
                            message_type = std::stoi(event["message_type"].get<std::string>());
                        }
                    }
                    
                    if (event.contains("timestamp")) {
                        if (event["timestamp"].is_number()) {
                            timestamp = event["timestamp"].get<int64_t>();
                        } else if (event["timestamp"].is_string()) {
                            timestamp = std::stoll(event["timestamp"].get<std::string>());
                        }
                    }
                    
                    message.set_message_id(message_id);
                    message.set_from_user_id(from_user_id);
                    message.set_to_user_id(to_user_id);
                    message.set_message_type(static_cast<im::MessageType>(message_type));
                    
                    if (event.contains("content") && event["content"].is_string()) {
                        message.set_content(event["content"].get<std::string>());
                    }
                    
                    message.set_send_time(timestamp);
                    message.set_is_read(false);
                    
                    if (event.contains("extra_info") && event["extra_info"].is_string()) {
                        message.set_extra_info(event["extra_info"].get<std::string>());
                    }
                    
                    message_service_->NotifyNewMessage(to_user_id, message);
                }
            }
        } 
        else if (event_type == "friend_request_sent" || 
                 event_type == "friend_request_accepted" || 
                 event_type == "friend_request_rejected" || 
                 event_type == "friend_deleted") {
            // 处理好友关系事件
            int64_t user_id = 0;
            
            if (event.contains("to_user_id")) {
                if (event["to_user_id"].is_number()) {
                    user_id = event["to_user_id"].get<int64_t>();
                } else if (event["to_user_id"].is_string()) {
                    user_id = std::stoll(event["to_user_id"].get<std::string>());
                }
            } else if (event.contains("user_id")) {
                if (event["user_id"].is_number()) {
                    user_id = event["user_id"].get<int64_t>();
                } else if (event["user_id"].is_string()) {
                    user_id = std::stoll(event["user_id"].get<std::string>());
                }
            }
            
            // 创建通知消息
            Message notification;
            notification.set_message_id(0);
            notification.set_from_user_id(0);  // 系统消息
            notification.set_to_user_id(user_id);
            notification.set_message_type(im::MessageType::TEXT);
            
            int64_t timestamp = utils::DateTime::NowSeconds();
            if (event.contains("timestamp")) {
                if (event["timestamp"].is_number()) {
                    timestamp = event["timestamp"].get<int64_t>();
                } else if (event["timestamp"].is_string()) {
                    timestamp = std::stoll(event["timestamp"].get<std::string>());
                }
            }
            
            notification.set_send_time(timestamp);
            notification.set_is_read(false);
            
            json extra;
            extra["type"] = "relationship_notification";
            extra["event_type"] = event_type;
            
            if (event_type == "friend_request_sent") {
                int64_t req_from_user_id = 0;
                if (event.contains("from_user_id")) {
                    if (event["from_user_id"].is_number()) {
                        req_from_user_id = event["from_user_id"].get<int64_t>();
                    } else if (event["from_user_id"].is_string()) {
                        req_from_user_id = std::stoll(event["from_user_id"].get<std::string>());
                    }
                }
                
                notification.set_content("用户 " + std::to_string(req_from_user_id) + " 发送了好友请求");
                extra["from_user_id"] = req_from_user_id;
                
                if (event.contains("request_id")) {
                    if (event["request_id"].is_number()) {
                        extra["request_id"] = event["request_id"].get<int64_t>();
                    } else if (event["request_id"].is_string()) {
                        extra["request_id"] = event["request_id"].get<std::string>();
                    }
                }
            } 
            else if (event_type == "friend_request_accepted") {
                int64_t req_to_user_id = 0;
                if (event.contains("to_user_id")) {
                    if (event["to_user_id"].is_number()) {
                        req_to_user_id = event["to_user_id"].get<int64_t>();
                    } else if (event["to_user_id"].is_string()) {
                        req_to_user_id = std::stoll(event["to_user_id"].get<std::string>());
                    }
                }
                
                notification.set_content("用户 " + std::to_string(req_to_user_id) + " 接受了您的好友请求");
                extra["to_user_id"] = req_to_user_id;
            } 
            else if (event_type == "friend_request_rejected") {
                int64_t req_to_user_id = 0;
                if (event.contains("to_user_id")) {
                    if (event["to_user_id"].is_number()) {
                        req_to_user_id = event["to_user_id"].get<int64_t>();
                    } else if (event["to_user_id"].is_string()) {
                        req_to_user_id = std::stoll(event["to_user_id"].get<std::string>());
                    }
                }
                
                notification.set_content("用户 " + std::to_string(req_to_user_id) + " 拒绝了您的好友请求");
                extra["to_user_id"] = req_to_user_id;
            } 
            else if (event_type == "friend_deleted") {
                int64_t friend_id = 0;
                if (event.contains("friend_id")) {
                    if (event["friend_id"].is_number()) {
                        friend_id = event["friend_id"].get<int64_t>();
                    } else if (event["friend_id"].is_string()) {
                        friend_id = std::stoll(event["friend_id"].get<std::string>());
                    }
                }
                
                notification.set_content("您与用户 " + std::to_string(friend_id) + " 的好友关系已解除");
                extra["friend_id"] = friend_id;
            }
            
            notification.set_extra_info(extra.dump());
            
            // 发送通知
            SendNotification(user_id, notification);
        } 
        else if (event_type == "file_transfer_request" || 
                 event_type == "file_transfer_accepted" || 
                 event_type == "file_transfer_rejected") {
            // 处理文件传输事件
            int64_t user_id = 0;
            
            if (event_type == "file_transfer_request") {
                if (event.contains("to_user_id")) {
                    if (event["to_user_id"].is_number()) {
                        user_id = event["to_user_id"].get<int64_t>();
                    } else if (event["to_user_id"].is_string()) {
                        user_id = std::stoll(event["to_user_id"].get<std::string>());
                    }
                }
            } else {
                if (event.contains("from_user_id")) {
                    if (event["from_user_id"].is_number()) {
                        user_id = event["from_user_id"].get<int64_t>();
                    } else if (event["from_user_id"].is_string()) {
                        user_id = std::stoll(event["from_user_id"].get<std::string>());
                    }
                }
            }
            
            // 创建通知消息
            Message notification;
            notification.set_message_id(0);
            notification.set_from_user_id(0);  // 系统消息
            notification.set_to_user_id(user_id);
            notification.set_message_type(im::MessageType::TEXT);
            
            int64_t timestamp = utils::DateTime::NowSeconds();
            if (event.contains("timestamp")) {
                if (event["timestamp"].is_number()) {
                    timestamp = event["timestamp"].get<int64_t>();
                } else if (event["timestamp"].is_string()) {
                    timestamp = std::stoll(event["timestamp"].get<std::string>());
                }
            }
            
            notification.set_send_time(timestamp);
            notification.set_is_read(false);
            
            json extra;
            extra["type"] = "file_notification";
            extra["event_type"] = event_type;
            
            if (event_type == "file_transfer_request") {
                int64_t req_from_user_id = 0;
                std::string file_name;
                
                if (event.contains("from_user_id")) {
                    if (event["from_user_id"].is_number()) {
                        req_from_user_id = event["from_user_id"].get<int64_t>();
                    } else if (event["from_user_id"].is_string()) {
                        req_from_user_id = std::stoll(event["from_user_id"].get<std::string>());
                    }
                }
                
                if (event.contains("file_name") && event["file_name"].is_string()) {
                    file_name = event["file_name"].get<std::string>();
                }
                
                notification.set_content("用户 " + std::to_string(req_from_user_id) + 
                                        " 想要发送文件: " + file_name);
                
                extra["from_user_id"] = req_from_user_id;
                
                if (event.contains("request_id")) {
                    if (event["request_id"].is_number()) {
                        extra["request_id"] = event["request_id"].get<int64_t>();
                    } else if (event["request_id"].is_string()) {
                        extra["request_id"] = event["request_id"].get<std::string>();
                    }
                }
                
                extra["file_name"] = file_name;
                
                if (event.contains("file_size")) {
                    if (event["file_size"].is_number()) {
                        extra["file_size"] = event["file_size"].get<int64_t>();
                    } else if (event["file_size"].is_string()) {
                        extra["file_size"] = event["file_size"].get<std::string>();
                    }
                }
            } 
            else if (event_type == "file_transfer_accepted") {
                int64_t req_to_user_id = 0;
                std::string file_name;
                
                if (event.contains("to_user_id")) {
                    if (event["to_user_id"].is_number()) {
                        req_to_user_id = event["to_user_id"].get<int64_t>();
                    } else if (event["to_user_id"].is_string()) {
                        req_to_user_id = std::stoll(event["to_user_id"].get<std::string>());
                    }
                }
                
                if (event.contains("file_name") && event["file_name"].is_string()) {
                    file_name = event["file_name"].get<std::string>();
                }
                
                notification.set_content("用户 " + std::to_string(req_to_user_id) + 
                                        " 接受了您的文件传输请求");
                
                extra["to_user_id"] = req_to_user_id;
                
                if (event.contains("file_id")) {
                    if (event["file_id"].is_number()) {
                        extra["file_id"] = event["file_id"].get<int64_t>();
                    } else if (event["file_id"].is_string()) {
                        extra["file_id"] = event["file_id"].get<std::string>();
                    }
                }
                
                extra["file_name"] = file_name;
            } 
            else if (event_type == "file_transfer_rejected") {
                int64_t req_to_user_id = 0;
                std::string file_name;
                
                if (event.contains("to_user_id")) {
                    if (event["to_user_id"].is_number()) {
                        req_to_user_id = event["to_user_id"].get<int64_t>();
                    } else if (event["to_user_id"].is_string()) {
                        req_to_user_id = std::stoll(event["to_user_id"].get<std::string>());
                    }
                }
                
                if (event.contains("file_name") && event["file_name"].is_string()) {
                    file_name = event["file_name"].get<std::string>();
                }
                
                notification.set_content("用户 " + std::to_string(req_to_user_id) + 
                                        " 拒绝了您的文件传输请求");
                
                extra["to_user_id"] = req_to_user_id;
                extra["file_name"] = file_name;
            }
            
            notification.set_extra_info(extra.dump());
            
            // 发送通知
            SendNotification(user_id, notification);
        }
        // 可以处理更多事件类型...
        
    } catch (const std::exception& e) {
        LOG_ERROR("处理Kafka事件失败: {}", e.what());
    }
}

void NotificationServiceImpl::AddActiveStream(int64_t user_id, grpc::ServerWriter<Message>* writer) {
    std::lock_guard<std::mutex> lock(streams_mutex_);
    active_streams_[user_id].push_back(writer);
    LOG_INFO("用户 {} 添加了活跃通知流，当前共 {} 个", user_id, active_streams_[user_id].size());
}

void NotificationServiceImpl::RemoveActiveStream(int64_t user_id, grpc::ServerWriter<Message>* writer) {
    std::lock_guard<std::mutex> lock(streams_mutex_);
    auto it = active_streams_.find(user_id);
    if (it != active_streams_.end()) {
        auto& streams = it->second;
        streams.erase(std::remove(streams.begin(), streams.end(), writer), streams.end());
        
        if (streams.empty()) {
            active_streams_.erase(it);
            LOG_INFO("用户 {} 的所有通知流已移除", user_id);
        } else {
            LOG_INFO("用户 {} 移除了一个通知流，当前还有 {} 个", user_id, streams.size());
        }
    }
}

} // namespace im 