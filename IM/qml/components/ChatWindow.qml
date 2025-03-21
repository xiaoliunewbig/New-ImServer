import QtQuick 6.0
import QtQuick.Controls 6.0

Column {
    spacing: Theme.spacingMedium

    ListView {
        id: chatHistory
        width: parent.width
        height: parent.height - sendArea.height - Theme.spacingMedium
        model: ListModel {}
        delegate: Text {
            text: model.message
            color: Theme.primaryTextColor
            wrapMode: Text.Wrap
        }
    }

    Row {
        id: sendArea
        spacing: Theme.spacingMedium

        TextField {
            id: messageField
            placeholderText: "输入消息"
            width: parent.width - sendButton.width - Theme.spacingMedium
            onTextChanged: validateInput()
        }

        Button {
            id: sendButton
            text: "发送"
            onClicked: {
                WebSocketManager.sendMessage({
                    type: "sendMessage",
                    data: {
                        message: messageField.text
                    }
                });
                messageField.text = "";
            }
        }

        WebSocketManager.onMessageReceived: function(message) {
            if (message.type === "sendMessage") {
                if (message.data.success) {
                    console.log("消息发送成功");
                } else {
                    console.error("消息发送失败: " + message.data.message);
                    showToast("消息发送失败: " + message.data.message);
                }
            }
        }
    }

    function validateInput() {
        sendButton.enabled = messageField.text.length > 0
    }

    function addMessage(message) {
        chatHistory.model.append({ message: message })
    }

    Connections {
        target: WebSocketManager
        onMessageReceived: function(message) {
            if (message.type === "message") {
                addMessage(message.data.message)
            }
        }
    }
}