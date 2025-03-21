#ifndef IM_MESSAGE_SERVICE_H
#define IM_MESSAGE_SERVICE_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include "proto/service.grpc.pb.h"
#include "db/mysql_connection.h"
#include "db/redis_client.h"
#include "kafka/kafka_producer.h"

namespace im {

/**
 * @brief 消息服务实现类，处理消息发送、接收等功能
 */
class MessageServiceImpl final : public MessageService::Service {
public:
    /**
     * @brief 构造函数
     * @param mysql_conn MySQL连接
     * @param redis_client Redis客户端
     * @param kafka_producer Kafka生产者
     */
    MessageServiceImpl(
        std::shared_ptr<db::MySQLConnection> mysql_conn,
        std::shared_ptr<db::RedisClient> redis_client,
        std::shared_ptr<kafka::KafkaProducer> kafka_producer
    );

    /**
     * @brief 析构函数
     */
    ~MessageServiceImpl() override = default;

    /**
     * @brief 发送消息
     * @param context RPC上下文
     * @param request 发送消息请求
     * @param response 发送消息响应
     * @return RPC状态
     */
    grpc::Status SendMessage(
        grpc::ServerContext* context,
        const SendMessageRequest* request,
        SendMessageResponse* response
    ) override;

    /**
     * @brief 获取消息历史
     * @param context RPC上下文
     * @param request 获取消息历史请求
     * @param response 获取消息历史响应
     * @return RPC状态
     */
    grpc::Status GetMessageHistory(
        grpc::ServerContext* context,
        const GetMessageHistoryRequest* request,
        GetMessageHistoryResponse* response
    ) override;

    /**
     * @brief 获取离线消息
     * @param context RPC上下文
     * @param request 获取离线消息请求
     * @param response 获取离线消息响应
     * @return RPC状态
     */
    grpc::Status GetOfflineMessages(
        grpc::ServerContext* context,
        const GetOfflineMessagesRequest* request,
        GetOfflineMessagesResponse* response
    ) override;

    /**
     * @brief 标记消息为已读
     * @param context RPC上下文
     * @param request 标记消息为已读请求
     * @param response 标记消息为已读响应
     * @return RPC状态
     */
    grpc::Status MarkMessageRead(
        grpc::ServerContext* context,
        const MarkMessageReadRequest* request,
        MarkMessageReadResponse* response
    ) override;

    /**
     * @brief 消息流
     * @param context RPC上下文
     * @param stream 双向消息流
     * @return RPC状态
     */
    grpc::Status MessageStream(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<Message, Message>* stream
    ) override;

    /**
     * @brief 通知用户有新消息
     * @param user_id 用户ID
     * @param message 消息
     */
    void NotifyNewMessage(int64_t user_id, const Message& message);

    /**
     * @brief 添加活跃消息流
     * @param user_id 用户ID
     * @param stream 消息流
     */
    void AddActiveStream(int64_t user_id, grpc::ServerReaderWriter<Message, Message>* stream);

    /**
     * @brief 移除活跃消息流
     * @param user_id 用户ID
     * @param stream 消息流
     */
    void RemoveActiveStream(int64_t user_id, grpc::ServerReaderWriter<Message, Message>* stream);

private:
    /**
     * @brief 从请求元数据中获取授权令牌
     * @param context RPC上下文
     * @return 授权令牌
     */
    std::string GetAuthToken(const grpc::ServerContext* context) const;

    /**
     * @brief 验证JWT令牌
     * @param token JWT令牌
     * @param user_id 输出参数，成功时返回用户ID
     * @return 有效返回true，无效返回false
     */
    bool ValidateToken(const std::string& token, int64_t& user_id) const;

    /**
     * @brief 存储消息到数据库
     * @param message 消息
     * @return 成功返回消息ID，失败返回0
     */
    int64_t StoreMessage(const Message& message);

    /**
     * @brief 缓存消息到Redis
     * @param chat_type 聊天类型（personal或group）
     * @param chat_id 聊天ID
     * @param message 消息
     */
    void CacheMessage(const std::string& chat_type, int64_t chat_id, const Message& message);

    /**
     * @brief 发送消息到Kafka
     * @param message 消息
     * @param chat_type 聊天类型（personal或group）
     */
    void SendMessageToKafka(const Message& message, const std::string& chat_type);

    /**
     * @brief 检查用户是否在线
     * @param user_id 用户ID
     * @return 在线返回true，离线返回false
     */
    bool IsUserOnline(int64_t user_id) const;

    /**
     * @brief 将离线消息存储到Redis和Kafka
     * @param user_id 用户ID
     * @param message 消息
     */
    void StoreOfflineMessage(int64_t user_id, const Message& message);

    /**
     * @brief 记录日志，调试用
     * @param message 日志消息
     */
    void LogDebug(const std::string& message) const;

    /**
     * @brief 记录错误日志
     * @param message 错误信息
     */
    void LogError(const std::string& message) const;

    std::shared_ptr<db::MySQLConnection> mysql_conn_;
    std::shared_ptr<db::RedisClient> redis_client_;
    std::shared_ptr<kafka::KafkaProducer> kafka_producer_;

    // 用户活跃消息流
    std::mutex streams_mutex_;
    std::unordered_map<int64_t, std::vector<grpc::ServerReaderWriter<Message, Message>*>> active_streams_;
};

} // namespace im

#endif // IM_MESSAGE_SERVICE_H 
 