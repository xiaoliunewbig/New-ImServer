import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "utils"
import "pages"

Page {
    id: mainPage

    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // 左侧导航栏
            Rectangle {
                Layout.preferredWidth: 80
                Layout.fillHeight: true
                color: Theme.sidebarColor

                Column {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    spacing: 20
                    topPadding: 30

                    ProfileAvatar {
                        id: userAvatar
                        width: 50
                        height: 50
                        anchors.horizontalCenter: parent.horizontalCenter
                        avatarUrl: WebSocketManager.userInfo.avatar || ""
                        username: WebSocketManager.userInfo.username || "User"
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                mainStackView.push("pages/SettingsPage.qml")
                            }
                        }
                    }

                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 30

                        // 聊天按钮
                        CustomButton {
                            id: chatButton
                            width: 50
                            height: 50
                            iconSource: "qrc:/assets/chat_icon.png"
                            selected: mainStackView.currentItem.objectName === "chatPage"
                            onClicked: {
                                if (mainStackView.currentItem.objectName !== "chatPage") {
                                    mainStackView.replace("pages/ChatPage.qml")
                                }
                            }
                        }

                        // 联系人按钮
                        CustomButton {
                            id: contactsButton
                            width: 50
                            height: 50
                            iconSource: "qrc:/assets/contacts_icon.png"
                            selected: mainStackView.currentItem.objectName === "contactsPage"
                            onClicked: {
                                if (mainStackView.currentItem.objectName !== "contactsPage") {
                                    mainStackView.replace("pages/ContactsPage.qml")
                                }
                            }
                        }

                        // 主题按钮
                        CustomButton {
                            id: themeButton
                            width: 50
                            height: 50
                            iconSource: "qrc:/assets/theme_icon.png"
                            selected: mainStackView.currentItem.objectName === "themePage"
                            onClicked: {
                                if (mainStackView.currentItem.objectName !== "themePage") {
                                    mainStackView.replace("pages/ThemePage.qml")
                                }
                            }
                        }

                        // 设置按钮
                        CustomButton {
                            id: settingsButton
                            width: 50
                            height: 50
                            iconSource: "qrc:/assets/settings_icon.png"
                            selected: mainStackView.currentItem.objectName === "settingsPage"
                            onClicked: {
                                if (mainStackView.currentItem.objectName !== "settingsPage") {
                                    mainStackView.replace("pages/SettingsPage.qml")
                                }
                            }
                        }
                    }
                }

                // 退出登录按钮
                CustomButton {
                    id: logoutButton
                    width: 50
                    height: 50
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    iconSource: "qrc:/assets/logout_icon.png"
                    onClicked: {
                        WebSocketManager.logout()
                        applicationWindow.navigateToLogin()
                    }
                }
            }

            // 主内容区域
            StackView {
                id: mainStackView
                Layout.fillWidth: true
                Layout.fillHeight: true
                initialItem: ChatPage {}
            }
        }
    }
}