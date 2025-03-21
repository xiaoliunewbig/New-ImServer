#ifndef IM_NOTIFICATION_SERVICE_H
#define IM_NOTIFICATION_SERVICE_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "proto/service.grpc.pb.h"
#include "db/mysql_connection.h"
#include "db/redis_client.h"
#include "kafka/kafka_producer.h"
#include "message_service.h"
#include "user_service.h"

namespace im {

/**
 * @brief 通知服务实现类，处理实时通知推送
 */
class NotificationServiceImpl final : public NotificationService::Service {
public:
    /**
     * @brief 构造函数
     * @param mysql_conn MySQL连接
     * @param redis_client Redis客户端
     * @param kafka_producer Kafka生产者
     */
    NotificationServiceImpl(
        std::shared_ptr<db::MySQLConnection> mysql_conn,
        std::shared_ptr<db::RedisClient> redis_client,
        std::shared_ptr<kafka::KafkaProducer> kafka_producer
    );

    /**
     * @brief 析构函数
     */
    ~NotificationServiceImpl() override;

    /**
     * @brief 订阅通知
     * @param context RPC上下文
     * @param request 订阅请求
     * @param writer 服务器到客户端的流
     * @return RPC状态
     */
    grpc::Status SubscribeNotifications(
        grpc::ServerContext* context,
        const SubscriptionRequest* request,
        grpc::ServerWriter<Message>* writer
    ) override;

    /**
     * @brief 设置消息服务引用
     * @param message_service 消息服务指针
     */
    void SetMessageService(MessageServiceImpl* message_service);

    /**
     * @brief 设置用户服务引用
     * @param user_service 用户服务指针
     */
    void SetUserService(UserServiceImpl* user_service);

    /**
     * @brief 向指定用户发送通知
     * @param user_id 用户ID
     * @param notification 通知消息
     */
    void SendNotification(int64_t user_id, const Message& notification);

    /**
     * @brief 向所有用户广播通知
     * @param notification 通知消息
     */
    void BroadcastNotification(const Message& notification);

private:
    // NotificationDoneCallback 类的前向声明
    class NotificationDoneCallback;

    /**
     * @brief 从请求元数据中获取授权令牌
     * @param context RPC上下文
     * @return 授权令牌
     */
    std::string GetAuthToken(grpc::ServerContext* context);

    /**
     * @brief 验证JWT令牌
     * @param context RPC上下文
     * @param user_id 输出参数，成功时返回用户ID
     * @return 有效返回true，无效返回false
     */
    bool ValidateToken(grpc::ServerContext* context, int64_t& user_id);

    /**
     * @brief 检查用户是否在线
     * @param user_id 用户ID
     * @return 在线返回true，离线返回false
     */
    bool IsUserOnline(int64_t user_id) const;

    /**
     * @brief Kafka消息处理线程函数
     */
    void KafkaConsumerThread();

    /**
     * @brief 处理Kafka事件
     * @param event_json 事件JSON
     */
    void HandleKafkaEvent(const std::string& event_json);

    /**
     * @brief 添加活跃通知流
     * @param user_id 用户ID
     * @param writer 消息流
     */
    void AddActiveStream(int64_t user_id, grpc::ServerWriter<Message>* writer);

    /**
     * @brief 移除活跃通知流
     * @param user_id 用户ID
     * @param writer 消息流
     */
    void RemoveActiveStream(int64_t user_id, grpc::ServerWriter<Message>* writer);

    std::shared_ptr<db::MySQLConnection> mysql_conn_;
    std::shared_ptr<db::RedisClient> redis_client_;
    std::shared_ptr<kafka::KafkaProducer> kafka_producer_;

    // 用户活跃通知流
    std::mutex streams_mutex_;
    std::unordered_map<int64_t, std::vector<grpc::ServerWriter<Message>*>> active_streams_;

    // 其他服务引用
    MessageServiceImpl* message_service_;
    UserServiceImpl* user_service_;

    // Kafka消费线程
    std::thread kafka_consumer_thread_;
    std::atomic<bool> running_;
};

} // namespace im

#endif // IM_NOTIFICATION_SERVICE_H 