import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "utils"

Page {
    id: loginPage

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
                source: "qrc:/assets/login_logo.png"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: "欢迎使用IM聊天"
                font.pixelSize: 24
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.primaryTextColor
            }

            TextField {
                id: usernameField
                width: parent.width
                placeholderText: "用户名或邮箱"
                font.pixelSize: 16
                leftPadding: 40
                background: Rectangle {
                    radius: 4
                    border.color: Theme.borderColor
                }

                Image {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: "qrc:/assets/user_icon.png"
                }
            }

            TextField {
                id: passwordField
                width: parent.width
                placeholderText: "密码"
                echoMode: TextInput.Password
                font.pixelSize: 16
                leftPadding: 40
                background: Rectangle {
                    radius: 4
                    border.color: Theme.borderColor
                }

                Image {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: "qrc:/assets/lock_icon.png"
                }
            }

            Button {
                text: "登录"
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
                    // 登录逻辑
                    if (WebSocketManager.login(usernameField.text, passwordField.text)) {
                        applicationWindow.navigateToMain()
                    } else {
                        errorMessage.text = "用户名或密码错误"
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

            Row {
                width: parent.width
                spacing: 10

                Text {
                    text: "注册账号"
                    color: Theme.linkColor
                    font.pixelSize: 14
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            stackView.push("RegisterWindow.qml")
                        }
                    }
                }

                Item { width: parent.width - registerText.width - forgetText.width - 20; height: 1 }

                Text {
                    id: forgetText
                    text: "忘记密码"
                    color: Theme.linkColor
                    font.pixelSize: 14
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            stackView.push("ResetPasswordWindow.qml")
                        }
                    }
                }
            }
        }
    }
}