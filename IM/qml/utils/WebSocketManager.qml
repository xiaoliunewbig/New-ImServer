pragma Singleton
import QtQuick 6.0
import QtWebSockets

QtObject {
    id: websocketManager

    // WebSocket 连接
    property WebSocket webSocket: WebSocket {
        url: "ws://localhost:8080/ws"
        onTextMessageReceived: function(message) {
            handleMessage(message)
        }
        onStatusChanged: function(status) {
            if (status === WebSocket.Open) {
                console.log("WebSocket connected")
            } else if (status === WebSocket.Closed) {
                console.log("WebSocket disconnected")
                reconnectTimer.start()
            }
        }
    }

    // 重连定时器
    property Timer reconnectTimer: Timer {
        interval: 3000
        repeat: false
        onTriggered: {
            if (webSocket.status === WebSocket.Closed) {
                webSocket.active = true
            }
        }
    }

    // 发送消息
    function sendMessage(message) {
        if (webSocket.status === WebSocket.Open) {
            webSocket.sendTextMessage(JSON.stringify(message))
        } else {
            console.error("WebSocket 未连接")
        }
    }

    // 处理接收到的消息
    function handleMessage(message) {
        try {
            var msg = JSON.parse(message)
            messageReceived(msg)
        } catch (e) {
            console.error("解析消息失败:", e)
        }
    }

    // 信号：接收到消息
    signal messageReceived(var message)

    property string connectionState: "disconnected" // disconnected, connecting, connected

    function updateConnectionState(state) {
        connectionState = state;
        console.log("WebSocket 连接状态更新: " + state);
    }

    webSocket.onStatusChanged: function(status) {
        if (status === WebSocket.Open) {
            updateConnectionState("connected");
        } else if (status === WebSocket.Closed) {
            updateConnectionState("disconnected");
            reconnectTimer.start();
        } else if (status === WebSocket.Connecting) {
            updateConnectionState("connecting");
        }
    }
}