pragma Singleton
import QtQuick 6.0
import QtWebSockets

QtObject {
    id: chatWebSocket

    // WebSocket 连接
    property WebSocket webSocket: WebSocket {
        url: "ws://localhost:8080/chat"
        onTextMessageReceived: function(message) {
            handleChatMessage(message)
        }
        onStatusChanged: function(status) {
            if (status === WebSocket.Open) {
                console.log("聊天 WebSocket 已连接")
            } else if (status === WebSocket.Closed) {
                console.log("聊天 WebSocket 已断开")
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

    // 发送聊天消息
    function sendChatMessage(chatId, content, isGroup, messageType) {
        var message = {
            type: "sendMessage",
            data: {
                chatId: chatId,
                content: content,
                isGroup: isGroup,
                messageType: messageType || "text"
            }
        }
        webSocket.sendTextMessage(JSON.stringify(message))
    }

    // 处理接收到的聊天消息
    function handleChatMessage(message) {
        try {
            var msg = JSON.parse(message)
            switch (msg.type) {
                case "message":
                    chatMessageReceived(msg.data)
                    break
                case "status":
                    userStatusUpdated(msg.data)
                    break
                case "notification":
                    notificationReceived(msg.data)
                    break
            }
        } catch (e) {
            console.error("解析聊天消息失败:", e)
        }
    }

    // 信号：接收到聊天消息
    signal chatMessageReceived(var message)

    // 信号：用户状态更新
    signal userStatusUpdated(var status)

    // 信号：接收到通知
    signal notificationReceived(var notification)
}