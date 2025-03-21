import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "utils"

Page {
    id: registerPage
    title: "注册账号"

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor

        Column {
            width: 400
            spacing: 20
            anchors.centerIn: parent

            Image {
                width: 120
                height: 120
                source: "qrc:/assets/register_logo.png"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: "创建账号"
                font.pixelSize: 24
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.primaryTextColor
            }

            TextField {
                id: usernameField
                width: parent.width
                placeholderText: "用户名"
                font.pixelSize: 16
            }

            TextField {
                id: emailField
                width: parent.width
                placeholderText: "邮箱"
                font.pixelSize: 16
            }

            TextField {
                id: passwordField
                width: parent.width
                placeholderText: "密码"
                echoMode: TextInput.Password
                font.pixelSize: 16
            }

            TextField {
                id: confirmPasswordField
                width: parent.width
                placeholderText: "确认密码"
                echoMode: TextInput.Password
                font.pixelSize: 16
            }

            Row {
                width: parent.width
                spacing: 10

                TextField {
                    id: verificationCodeField
                    width: parent.width - sendCodeButton.width - 10
                    placeholderText: "验证码"
                    font.pixelSize: 16
                }

                Button {
                    id: sendCodeButton
                    text: countdown > 0 ? "重新发送(" + countdown + "s)" : "发送验证码"
                    enabled: countdown <= 0
                    
                    property int countdown: 0
                    
                    onClicked: {
                        // 发送验证码逻辑
                        WebSocketManager.sendVerificationCode(emailField.text)
                        countdown = 60
                        countdownTimer.start()
                    }
                    
                    Timer {
                        id: countdownTimer
                        interval: 1000
                        repeat: true
                        onTriggered: {
                            if (sendCodeButton.countdown > 0) {
                                sendCodeButton.countdown--
                            } else {
                                stop()
                            }
                        }
                    }
                }
            }

            Button {
                text: "注册"
                width: parent.width
                height: 50
                font.pixelSize: 16
                background: Rectangle {
                    color: parent.down ? Qt.darker(Theme.primaryColor, 1.1) : Theme.primaryColor
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    // 注册逻辑
                    if (passwordField.text !== confirmPasswordField.text) {
                        errorMessage.text = "两次输入的密码不一致"
                        errorMessage.visible = true
                        return
                    }
                    
                    if (WebSocketManager.register(
                        usernameField.text, 
                        emailField.text, 
                        passwordField.text, 
                        verificationCodeField.text)) {
                        stackView.pop() // 返回登录页
                    } else {
                        errorMessage.text = "注册失败，请检查输入"
                        errorMessage.visible = true
                    }
                }
            }

            Text {
                id: errorMessage
                color: "red"
                font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
                visible: false
            }

            Text {
                text: "返回登录"
                color: Theme.linkColor
                font.pixelSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        stackView.pop()
                    }
                }
            }
        }
    }
}