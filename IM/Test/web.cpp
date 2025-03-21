#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>

typedef websocketpp::server<websocketpp::config::asio> server;

void on_open(server* s, websocketpp::connection_hdl hdl) {
    std::cout << "WebSocket 连接成功" << std::endl;
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    std::cout << "客户端地址: " << con->get_remote_endpoint() << std::endl;
}

void on_fail(server* s, websocketpp::connection_hdl hdl) {
    std::cerr << "WebSocket 连接失败" << std::endl;
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    std::cerr << "错误信息: " << con->get_ec().message() << std::endl;
}

void on_message(server* s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::cout << "收到消息: " << msg->get_payload() << std::endl;
    s->send(hdl, "服务器已收到: " + msg->get_payload(), msg->get_opcode());
}

int main() {
    server ws_server;
    ws_server.set_access_channels(websocketpp::log::alevel::all);
    ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

    ws_server.init_asio();
    ws_server.set_open_handler(std::bind(&on_open, &ws_server, std::placeholders::_1));
    ws_server.set_fail_handler(std::bind(&on_fail, &ws_server, std::placeholders::_1));
    ws_server.set_message_handler(std::bind(&on_message, &ws_server, std::placeholders::_1, std::placeholders::_2));

    try {
        int port = 8080;
        while (true) {
            try {
                ws_server.listen(port);
                std::cout << "WebSocket 服务器已启动，监听端口: " << port << std::endl;
                break;
            } catch (const websocketpp::exception& e) {
                std::cerr << "端口 " << port << " 已被占用，尝试下一个端口..." << std::endl;
                port++;
            }
        }

        ws_server.start_accept();
        ws_server.run();
    } catch (const websocketpp::exception& e) {
        std::cerr << "WebSocket 服务器启动失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
