import QtQuick 6.0
import QtQuick.Controls 6.0

Column {
    spacing: Theme.spacingMedium

    TextField {
        id: emailField
        placeholderText: "邮箱"
        width: parent.width
        onTextChanged: validateInput()
    }

    TextField {
        id: verificationCodeField
        placeholderText: "验证码"
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
        id: registerButton
        text: "注册"
        width: parent.width
        enabled: false
        onClicked: {
            WebSocketManager.sendMessage({
                type: "register",
                data: {
                    email: emailField.text,
                    verificationCode: verificationCodeField.text,
                    password: passwordField.text
                }
            });
        }
    }

    WebSocketManager.onMessageReceived: function(message) {
        if (message.type === "register") {
            if (message.data.success) {
                console.log("注册成功");
                navigateToLogin();
            } else {
                console.error("注册失败: " + message.data.message);
                showToast("注册失败: " + message.data.message);
            }
        }
    }

    function validateInput() {
        registerButton.enabled = emailField.text.length > 0 && verificationCodeField.text.length > 0 && passwordField.text.length > 0
    }
}