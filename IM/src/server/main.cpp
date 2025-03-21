#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <boost/program_options.hpp>
#include "server/server.h"
#include "server/utils/logger.h"
#include "absl/time/clock.h" // 添加 absl 时间库

namespace po = boost::program_options;

// 全局服务器实例
std::unique_ptr<im::IMServer> g_server;
// 信号标志
volatile sig_atomic_t g_shutdown_flag = 0;

// 信号处理函数
void signal_handler(int signal) {
    LOG_INFO("接收到信号 {}，准备关闭服务器...", signal);
    g_shutdown_flag = 1;
}

void ShutdownServer() {
    LOG_INFO("正在关闭服务器...");
    LOG_INFO("Stopping IM server...");

    // 并行关闭 gRPC 和 WebSocket 服务器
    std::thread grpc_shutdown_thread([&]() {
        LOG_INFO("Shutting down gRPC server...");
        if (g_server && g_server->GetGrpcServer()) {
            // 使用 std::chrono::system_clock::now() + std::chrono::seconds(1)
            g_server->GetGrpcServer()->Shutdown(
                std::chrono::system_clock::now() + std::chrono::seconds(1)
            );
        }
    });

    std::thread websocket_shutdown_thread([&]() {
        LOG_INFO("Shutting down WebSocket server...");
        if (g_server) {
            g_server->StopWebSocketServer();
        }
    });

    // 等待关闭完成
    grpc_shutdown_thread.join();
    websocket_shutdown_thread.join();

    LOG_INFO("服务器已关闭");
}

int main(int argc, char* argv[]) {
    try {
        // 解析命令行参数
        std::string config_path;
        
        po::options_description desc("即时通信系统服务器");
        desc.add_options()
            ("help,h", "显示帮助信息")
            ("config,c", po::value<std::string>(&config_path)->default_value("/home/pck/markdown/重构/IM/conf/server.jsonc"), "配置文件路径")
            ("log-level,l", po::value<std::string>()->default_value("info"), "日志级别 (trace, debug, info, warning, error, critical, off)");
        
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }
        
        // 初始化日志
        std::string log_level = vm["log-level"].as<std::string>();
        im::utils::Logger::Initialize(log_level);
        
        LOG_INFO("即时通信系统服务器启动中...");
        LOG_INFO("配置文件: {}", config_path);
        
        // 设置信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        // 创建并启动服务器
        g_server = std::make_unique<im::IMServer>(config_path);
        
        if (!g_server->Start()) {
            LOG_CRITICAL("服务器启动失败！");
            return 1;
        }
        
        LOG_INFO("服务器启动成功，等待连接...");
        
        // 主循环
        while (!g_shutdown_flag) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // 停止服务器
        LOG_INFO("正在关闭服务器...");
        g_server->Stop();
        LOG_INFO("服务器已关闭");
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        LOG_CRITICAL("服务器异常终止: {}", e.what());
        return 1;
    }
}