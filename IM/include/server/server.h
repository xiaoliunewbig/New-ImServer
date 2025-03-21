#ifndef IM_SERVER_H
#define IM_SERVER_H

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include <boost/asio.hpp>
#include "user_service.h"
#include "message_service.h"
#include "relation_service.h"
#include "file_service.h"
#include "notification_service.h"
#include "admin_service.h"
#include "db/mysql_connection.h"
#include "db/redis_client.h"
#include "kafka/kafka_producer.h"
#include "utils/string_util.h"
#include "websocket_handler.h"

namespace im {

/**
 * @brief IM系统服务器类，负责管理所有服务和连接
 */
class IMServer {
public:
    /**
     * @brief 构造函数
     * @param config_path 配置文件路径
     */
    explicit IMServer(const std::string& config_path);

    /**
     * @brief 析构函数
     */
    ~IMServer();

    /**
     * @brief 启动服务器
     * @return 成功返回true，失败返回false
     */
    bool Start();

    /**
     * @brief 停止服务器
     */
    void Stop();

    void StopWebSocketServer();

    /**
     * @brief 获取服务器状态
     * @return 服务器状态描述
     */
    std::string GetStatus() const;

    /**
     * @brief 获取活跃连接数
     * @return 活跃连接数
     */
    size_t GetActiveConnectionCount() const;

    /**
     * @brief 强制断开用户连接
     * @param user_id 用户ID
     * @return 成功返回true，失败返回false
     */
    bool DisconnectUser(int64_t user_id);

    /**
     * @brief 获取日志
     * @param level 日志级别
     * @param limit 限制条数
     * @param offset 偏移量
     * @return 日志内容
     */
    std::string GetLogs(int level, int limit, int offset) const;

    /**
     * @brief 向指定用户发送WebSocket消息
     * @param user_id 用户ID
     * @param message 消息内容
     * @return 成功返回true，失败返回false
     */
    bool SendWebSocketMessage(int64_t user_id, const std::string& message);

    /**
     * @brief 向所有用户广播WebSocket消息
     * @param message 消息内容
     */
    void BroadcastWebSocketMessage(const std::string& message);

        // 获取 gRPC 服务器实例
        std::unique_ptr<grpc::Server>& GetGrpcServer() {
            return grpc_server_;
        }

        const std::unique_ptr<grpc::Server>& GetGrpcServer() const {
            return grpc_server_;
        }
    
private:
    /**
     * @brief 初始化服务
     * @return 成功返回true，失败返回false
     */
    bool InitServices();

    /**
     * @brief 初始化数据库连接
     * @return 成功返回true，失败返回false
     */
    bool InitDatabase();

    /**
     * @brief 初始化Redis连接
     * @return 成功返回true，失败返回false
     */
    bool InitRedis();

    /**
     * @brief 初始化Kafka
     * @return 成功返回true，失败返回false
     */
    bool InitKafka();

    /**
     * @brief 注册服务实现
     */
    void RegisterServices();

    /**
     * @brief 初始化WebSocket处理器
     * @return 成功返回true，失败返回false
     */
    bool InitWebSocketHandler();
    
    /**
     * @brief 启动WebSocket服务
     */
    void StartWebSocketServer();

    /**
     * @brief WebSocket服务器线程
     */
    void WebSocketServerThread();

    /**
     * @brief 处理新连接
     * @param socket 新连接的socket
     */
    void HandleNewWebSocketConnection(boost::asio::ip::tcp::socket socket);

    /**
     * @brief 管理连接活跃状态
     */
    void ManageConnections();

    void DoAccept();

    // 配置信息
    std::string config_path_;
    int grpc_port_;
    int websocket_port_;
    std::string mysql_host_;
    int mysql_port_;
    std::string mysql_user_;
    std::string mysql_password_;
    std::string mysql_database_;
    std::string redis_host_;
    int redis_port_;
    std::string kafka_brokers_;

    // 服务状态
    std::atomic<bool> running_;
    int64_t server_start_time_;  // 服务器启动时间戳
    mutable std::mutex connection_mutex_;
    std::unordered_map<int64_t, std::vector<std::string>> active_connections_;  // user_id -> connection_ids

    // 服务器实例
    std::unique_ptr<grpc::Server> grpc_server_;
    boost::asio::io_context io_context_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::vector<std::thread> websocket_thread_; // 支持多个线程
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;
    std::shared_ptr<WebSocketHandler> websocket_handler_;

    // 数据库连接
    std::shared_ptr<db::MySQLConnection> mysql_connection_;
    std::shared_ptr<db::RedisClient> redis_client_;
    std::shared_ptr<kafka::KafkaProducer> kafka_producer_;

    // 服务实现
    std::unique_ptr<UserServiceImpl> user_service_;
    std::unique_ptr<MessageServiceImpl> message_service_;
    std::unique_ptr<RelationServiceImpl> relation_service_;
    std::unique_ptr<FileServiceImpl> file_service_;
    std::unique_ptr<NotificationServiceImpl> notification_service_;
    std::unique_ptr<AdminServiceImpl> admin_service_;


};

} // namespace im

#endif // IM_SERVER_H