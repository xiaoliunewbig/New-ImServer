#include "server/server.h"
#include "server/utils/config.h"
#include "server/utils/logger.h"
#include "server/utils/datetime.h"
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server_builder.h>
#include <string>
#include <thread>
#include <chrono>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>

namespace im {

IMServer::IMServer(const std::string& config_path)
    : config_path_(config_path),
      running_(false),
      server_start_time_(utils::DateTime::NowSeconds()) {
    LOG_INFO("Initializing IM server with config: {}", config_path);
}

IMServer::~IMServer() {
    Stop();
}

bool IMServer::Start() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (running_) {
        LOG_WARN("Server is already running");
        return true;
    }
    
    try {
        // 记录服务器启动时间
        server_start_time_ = utils::DateTime::NowSeconds();
        
        LOG_INFO("Loading configuration...");
        if (!utils::Config::GetInstance().Load(config_path_)) {
            LOG_CRITICAL("Failed to load configuration from: {}", config_path_);
            return false;
        }

        // 读取配置
        grpc_port_ = utils::Config::GetInstance().GetInt("server.port", 50051);
        websocket_port_ = utils::Config::GetInstance().GetInt("websocket.port", 8080);
        mysql_host_ = utils::Config::GetInstance().GetString("database.mysql.host", "localhost");
        mysql_port_ = utils::Config::GetInstance().GetInt("database.mysql.port", 3308);
        mysql_user_ = utils::Config::GetInstance().GetString("database.mysql.user", "root");
        mysql_password_ = utils::Config::GetInstance().GetString("database.mysql.password", "");
        mysql_database_ = utils::Config::GetInstance().GetString("database.mysql.database", "im_db");
        redis_host_ = utils::Config::GetInstance().GetString("database.redis.host", "localhost");
        redis_port_ = utils::Config::GetInstance().GetInt("database.redis.port", 6380);
        kafka_brokers_ = utils::Config::GetInstance().GetString("kafka.brokers", "localhost:9092");
        
        LOG_INFO("Initializing database connections...");
        if (!InitDatabase()) {
            LOG_CRITICAL("Failed to initialize database connections");
            return false;
        }
        
        LOG_INFO("Initializing Redis client...");
        if (!InitRedis()) {
            LOG_CRITICAL("Failed to initialize Redis client");
            return false;
        }
        
        LOG_INFO("Initializing Kafka...");
        if (!InitKafka()) {
            LOG_CRITICAL("Failed to initialize Kafka");
            return false;
        }
        
        LOG_INFO("Initializing services...");
        if (!InitServices()) {
            LOG_CRITICAL("Failed to initialize services");
            return false;
        }
        
        LOG_INFO("Starting gRPC server on port {}...", grpc_port_);
        RegisterServices();
        
        // 启动gRPC服务器
        grpc::ServerBuilder builder;
        builder.AddListeningPort("0.0.0.0:" + std::to_string(grpc_port_), grpc::InsecureServerCredentials());
        builder.RegisterService(user_service_.get());
        builder.RegisterService(message_service_.get());
        builder.RegisterService(relation_service_.get());
        builder.RegisterService(file_service_.get());
        builder.RegisterService(notification_service_.get());
        builder.RegisterService(admin_service_.get());
        
        // 设置gRPC服务器选项
        builder.SetMaxReceiveMessageSize(100 * 1024 * 1024); // 100MB
        builder.SetMaxSendMessageSize(100 * 1024 * 1024); // 100MB
        builder.SetDefaultCompressionLevel(GRPC_COMPRESS_LEVEL_HIGH);
        
        // 设置keepalive参数 - 使用正确的方式添加
        int keepalive_time_ms = utils::Config::GetInstance().GetInt("server.keepalive_time_ms", 20000);
        int keepalive_timeout_ms = utils::Config::GetInstance().GetInt("server.keepalive_timeout_ms", 10000);
        
        // 添加各种服务器参数
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, keepalive_time_ms);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, keepalive_timeout_ms);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        
        // 启动gRPC服务器
        grpc_server_ = builder.BuildAndStart();
        if (!grpc_server_) {
            LOG_CRITICAL("Failed to start gRPC server");
            return false;
        }
        
        // 启动WebSocket服务器
        if (utils::Config::GetInstance().GetBool("websocket.enabled", true)) {
            LOG_INFO("Starting WebSocket server on port {}...", websocket_port_);
            StartWebSocketServer();
        }
        
        running_ = true;
        LOG_INFO("IM server started successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Failed to start server: {}", e.what());
        return false;
    }
}

void IMServer::Stop() {
    std::lock_guard<std::mutex> lock(connection_mutex_);

    if (!running_) {
        return;
    }

    LOG_INFO("Stopping IM server...");

    running_ = false;

    // 停止gRPC服务器
    if (grpc_server_) {
        LOG_INFO("Shutting down gRPC server...");
        grpc_server_->Shutdown();
    }

    // 停止WebSocket服务器
    StopWebSocketServer();

    // 清理连接
    active_connections_.clear();

    LOG_INFO("IM server stopped");
}

void IMServer::StopWebSocketServer() {
    if (acceptor_ && acceptor_->is_open()) {
        LOG_INFO("Shutting down WebSocket server...");
        boost::system::error_code ec;
        acceptor_->close(ec);
        io_context_.stop();

        // 等待所有线程结束
        for (auto& thread : websocket_thread_) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        // 清理WebSocket处理器
        websocket_handler_.reset();

        websocket_thread_.clear(); // 清空线程容器
    }
}


std::string IMServer::GetStatus() const {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    std::string status = "IM Server Status:\n";
    
    status += "Running: " + std::string(running_ ? "Yes" : "No") + "\n";
    status += "Active Connections: " + std::to_string(GetActiveConnectionCount()) + "\n";
    status += "Started at: " + utils::DateTime::FormatTimestamp(server_start_time_) + "\n";
    status += "Uptime: " + std::to_string((utils::DateTime::NowSeconds() - server_start_time_) / 60) + " minutes\n";
    
    return status;
}

size_t IMServer::GetActiveConnectionCount() const {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    size_t total = 0;
    for (const auto& user_connections : active_connections_) {
        total += user_connections.second.size();
    }
    
    return total;
}

bool IMServer::DisconnectUser(int64_t user_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    auto it = active_connections_.find(user_id);
    if (it == active_connections_.end()) {
        LOG_WARN("User {} is not connected", user_id);
        return false;
    }
    
    LOG_INFO("Forcibly disconnecting user {}", user_id);
    active_connections_.erase(it);
    
    return true;
}

std::string IMServer::GetLogs(int level, int limit, int offset) const {
    // 简单实现，实际项目中应该从数据库读取日志
    return "Log retrieval not implemented yet";
}

bool IMServer::InitServices() {
    try {
        // 创建服务实例
        user_service_ = std::make_unique<UserServiceImpl>(mysql_connection_, redis_client_, kafka_producer_);
        message_service_ = std::make_unique<MessageServiceImpl>(mysql_connection_, redis_client_, kafka_producer_);
        relation_service_ = std::make_unique<RelationServiceImpl>(mysql_connection_, redis_client_, kafka_producer_);
        file_service_ = std::make_unique<FileServiceImpl>(mysql_connection_, redis_client_, kafka_producer_);
        notification_service_ = std::make_unique<NotificationServiceImpl>(mysql_connection_, redis_client_, kafka_producer_);
        admin_service_ = std::make_unique<AdminServiceImpl>(mysql_connection_, redis_client_, kafka_producer_);
        
        LOG_INFO("All services initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Failed to initialize services: {}", e.what());
        return false;
    }
}

bool IMServer::InitDatabase() {
    try {
        // 创建MySQL连接
        mysql_connection_ = std::make_shared<db::MySQLConnection>(
            mysql_host_,
            mysql_port_,
            mysql_user_,
            mysql_password_,
            mysql_database_
        );
        
        if (!mysql_connection_->Connect()) {
            LOG_CRITICAL("Failed to connect to MySQL: {}", mysql_connection_->GetLastError());
            return false;
        }
        
        LOG_INFO("Connected to MySQL database {}", mysql_database_);
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Failed to initialize database: {}", e.what());
        return false;
    }
}

bool IMServer::InitRedis() {
    try {
        // 创建Redis客户端
        std::string redis_password = utils::Config::GetInstance().GetString("database.redis.password", "");
        
        redis_client_ = std::make_shared<db::RedisClient>(
            redis_host_,
            redis_port_,
            redis_password
        );
        
        if (!redis_client_->Connect()) {
            LOG_ERROR("Failed to connect to Redis");
            return false;
        }
        
        LOG_INFO("Connected to Redis server {}:{}", redis_host_, redis_port_);
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Failed to initialize Redis: {}", e.what());
        return false;
    }
}

bool IMServer::InitKafka() {
    try {
        // 从配置获取更多参数
        std::string security_protocol = utils::Config::GetInstance().GetString(
            "kafka.security_protocol", "PLAINTEXT");
        int message_timeout_ms = utils::Config::GetInstance().GetInt(
            "kafka.message_timeout_ms", 30000);
        bool enable_idempotence = utils::Config::GetInstance().GetBool(
            "kafka.enable_idempotence", true);
        int max_in_flight = utils::Config::GetInstance().GetInt(
            "kafka.max_in_flight", 5);

        // 替换为跨平台的系统信息获取方式
        std::string hostname = boost::asio::ip::host_name(); // 使用Boost获取主机名
        auto pid = getpid(); // 使用标准库获取进程ID
        
        std::string client_id = "im_server_" + hostname + "_" 
                               + std::to_string(pid);

        // 使用配置参数初始化 Kafka
        kafka_producer_ = std::make_shared<im::kafka::KafkaProducer>(
            kafka_brokers_,
            client_id,
            [](const std::string& topic, const std::string& payload, bool success) {
                if (!success) {
                    LOG_ERROR("Kafka发送失败: topic={}, payload_size={}", topic, payload.size());
                } else {
                    LOG_DEBUG("Kafka消息已确认: topic={}", topic);
                }
            }
        );

        // 增加重试逻辑
        const int max_retries = 3;
        int retry_count = 0;
        while (!kafka_producer_->Initialize() && retry_count < max_retries) {
            LOG_WARN("Kafka初始化失败，第{}次重试...", retry_count + 1);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            retry_count++;
        }

        // 替换连接状态检查方式
        if (!kafka_producer_->IsValid()) { // 使用实际存在的方法名
            LOG_CRITICAL("无法连接Kafka brokers: {}", kafka_brokers_);
            return false;
        }

        LOG_INFO("成功连接Kafka集群 [brokers={}, client_id={}, protocol={}]", 
                kafka_brokers_, client_id, security_protocol);
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Kafka初始化失败: {} [brokers={}]", e.what(), kafka_brokers_);
        return false;
    }
}

void IMServer::RegisterServices() {
    // 注册各服务间的依赖和回调
    notification_service_->SetMessageService(message_service_.get());
    notification_service_->SetUserService(user_service_.get());
    
    LOG_INFO("Service dependencies registered");
}

bool IMServer::InitWebSocketHandler() {
    try {
        // 创建WebSocket处理器
        websocket_handler_ = std::make_shared<WebSocketHandler>(redis_client_);
        LOG_INFO("WebSocket handler initialized");
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Failed to initialize WebSocket handler: {}", e.what());
        return false;
    }
}

void IMServer::StartWebSocketServer() {
    // 初始化 acceptor_
    try {
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            io_context_,
            boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string("0.0.0.0"),
                websocket_port_
            )
        );
    } catch (const std::exception& e) {
        LOG_CRITICAL("Failed to initialize acceptor: {}", e.what());
        return;
    }

    if (!acceptor_) {
        LOG_CRITICAL("Failed to start WebSocket server: acceptor not initialized");
        return;
    }

    // 初始化 WebSocket 处理器
    if (!InitWebSocketHandler()) {
        LOG_CRITICAL("Failed to start WebSocket server: handler initialization failed");
        return;
    }

    // 获取线程数配置，默认为 4
    const int num_threads = utils::Config::GetInstance().GetInt("websocket.threads", 4);

    // 启动一个线程绑定端口，其他线程运行 io_context
    websocket_thread_.push_back(std::thread([this]() {
        try {
            LOG_INFO("WebSocket server listening on 0.0.0.0:" + std::to_string(websocket_port_));
            acceptor_->listen();
            DoAccept();
            io_context_.run();
        } catch (const std::exception& e) {
            LOG_ERROR("WebSocket server error: " + std::string(e.what()));
        }
    }));
    for (int i = 1; i < num_threads; ++i) {
        websocket_thread_.push_back(std::thread([this]() {
            try {
                io_context_.run();
            } catch (const std::exception& e) {
                LOG_ERROR("WebSocket server error: " + std::string(e.what()));
            }
        }));
    }

    LOG_INFO("WebSocket server started with {} threads", num_threads);
}



void IMServer::WebSocketServerThread() {
    try {
        // 配置WebSocket服务器
        std::string ws_host = utils::Config::GetInstance().GetString("websocket.host", "0.0.0.0");
        std::string ws_path = utils::Config::GetInstance().GetString("websocket.path", "/ws");
        
        // 创建acceptor
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            io_context_,
            boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string(ws_host),
                websocket_port_
            )
        );
        
        // 开始接受连接
        LOG_INFO("WebSocket server listening on {}:{}{}",
                ws_host, websocket_port_, ws_path);
        
    
        // 异步接受连接
        DoAccept();

        // 运行 io_context
        io_context_.run();
    } catch (const std::exception& e) {
        LOG_ERROR("WebSocket server error: {}", e.what());
    }        
                //     while (running_) {
    //         boost::asio::ip::tcp::socket socket(io_context_);
    //         acceptor_->accept(socket);
            
    //         if (running_) {
    //             // 使用WebSocketHandler处理新连接
    //             websocket_handler_->HandleNewConnection(std::move(socket));
    //         }
    //     }
    // } catch (const std::exception& e) {
    //     LOG_ERROR("WebSocket server error: {}", e.what());
    // }
}

bool IMServer::SendWebSocketMessage(int64_t user_id, const std::string& message) {
    if (!websocket_handler_) {
        LOG_ERROR("WebSocket handler not initialized");
        return false;
    }
    
    return websocket_handler_->SendToUser(user_id, message);
}

void IMServer::BroadcastWebSocketMessage(const std::string& message) {
    if (!websocket_handler_) {
        LOG_ERROR("WebSocket handler not initialized");
        return;
    }
    
    websocket_handler_->Broadcast(message);
}

void IMServer::ManageConnections() {
    // 周期性检查连接活跃状态
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    // 检查每个连接是否仍然活跃
    for (auto it = active_connections_.begin(); it != active_connections_.end();) {
        // 清理失效的连接
        // 实际实现中应该检查每个连接的最后活跃时间
        
        if (it->second.empty()) {
            it = active_connections_.erase(it);
        } else {
            ++it;
        }
    }
}

void IMServer::DoAccept() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);

    acceptor_->async_accept(*socket, [this, socket](boost::system::error_code ec) {
        if (ec) {
            LOG_ERROR("Accept connection failed: {}", ec.message());
            if (ec == boost::asio::error::operation_aborted) {
                LOG_INFO("Acceptor stopped, shutting down...");
                return;
            }
        } else {
            // 记录新连接的客户端信息
            LOG_INFO("New connection accepted from {}:{}",
                     socket->remote_endpoint().address().to_string(),
                     socket->remote_endpoint().port());

            // 使用WebSocketHandler处理新连接
            websocket_handler_->HandleNewConnection(std::move(*socket));
        }

        // 继续接受下一个连接
        if (running_) {
            DoAccept();
        }
    });
}

} // namespace im