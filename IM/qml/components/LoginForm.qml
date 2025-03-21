import QtQuick 6.0
import QtQuick.Controls 6.0

Column {
    spacing: Theme.spacingMedium

    TextField {
        id: usernameField
        placeholderText: "用户名"
        width: parent.width
        onTextChanged: validateInput()
    }

    TextField {
        id: passwordField
        placeholderText: "密码"
        echoMode: TextInput.Password
        width: parent.width
        onTextChanged: validateInput()
    }

    Button {
        id: loginButton
        text: "登录"
        width: parent.width
        enabled: false
        onClicked: {
            WebSocketManager.sendMessage({
                type: "login",
                data: {
                    username: usernameField.text,
                    password: passwordField.text
                }
            });
        }
    }

    function validateInput() {
        loginButton.enabled = usernameField.text.length > 0 && passwordField.text.length > 0
    }

    WebSocketManager.onMessageReceived: function(message) {
        if (message.type === "login") {
            if (message.data.success) {
                console.log("登录成功");
                navigateToMain();
            } else {
                console.error("登录失败: " + message.data.message);
                showToast("登录失败: " + message.data.message);
            }
        }
    }
}