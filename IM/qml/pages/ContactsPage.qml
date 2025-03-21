import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"
import "../utils"

Page {
    id: contactsPage
    objectName: "contactsPage"
    
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            
            // 标题栏
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: Theme.headerColor
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    
                    Label {
                        text: "联系人"
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.primaryTextColor
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Button {
                        icon.source: "qrc:/assets/add_icon.png"
                        flat: true
                        onClicked: {
                            addContactDialog.open()
                        }
                        
                        ToolTip.visible: hovered
                        ToolTip.text: "添加联系人"
                    }
                }
            }
            
            // 搜索栏
            Rectangle {
                Layout.fillWidth: true
                height: 60
                color: "transparent"
                
                TextField {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    height: 40
                    placeholderText: "搜索联系人"
                    leftPadding: 40
                    
                    background: Rectangle {
                        radius: 20
                        color: Theme.inputBackgroundColor
                    }
                    
                    Image {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 16
                        height: 16
                        source: "qrc:/assets/search_icon.png"
                    }
                }
            }
            
            // 联系人列表
            TabBar {
                id: contactTabBar
                Layout.fillWidth: true
                height: 50
                
                TabButton {
                    text: "好友"
                    width: implicitWidth
                }
                
                TabButton {
                    text: "群组"
                    width: implicitWidth
                }
                
                TabButton {
                    text: "新的朋友"
                    width: implicitWidth
                }
            }
            
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: contactTabBar.currentIndex
                
                // 好友列表
                ListView {
                    id: friendsListView
                    clip: true
                    model: WebSocketManager.contacts
                    
                    // 字母索引
                    section.property: "username"
                    section.criteria: ViewSection.FirstCharacter
                    section.delegate: Rectangle {
                        width: friendsListView.width
                        height: 30
                        color: Theme.secondaryBackgroundColor
                        
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: section
                            font.bold: true
                            color: Theme.secondaryTextColor
                        }
                    }
                    
                    delegate: Rectangle {
                        width: friendsListView.width
                        height: 60
                        color: mouseArea.containsMouse ? Theme.highlightColor : "transparent"
                        
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                // 打开聊天页面
                                mainStackView.replace("ChatPage.qml")
                                var chatPage = mainStackView.currentItem
                                chatPage.currentChat = modelData
                                chatPage.isGroupChat = false
                                WebSocketManager.loadChatHistory(modelData.id, false)
                            }
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            spacing: 15
                            
                            // 头像
                            Image {
                                source: modelData.avatar || "qrc:/assets/default_avatar.png"
                                width: 40
                                height: 40
                                Layout.alignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectCrop
                                layer.enabled: true
                                layer.effect: OpacityMask {
                                    maskSource: Rectangle {
                                        width: 40
                                        height: 40
                                        radius: 20
                                    }
                                }
                            }
                            
                            // 用户信息
                            Column {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 5
                                
                                Text {
                                    text: modelData.username
                                    font.pixelSize: 16
                                    color: Theme.primaryTextColor
                                }
                                
                                Text {
                                    text: getStatusText(modelData.status)
                                    font.pixelSize: 12
                                    color: Theme.secondaryTextColor
                                }
                            }
                            
                            // 状态指示器
                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                Layout.alignment: Qt.AlignVCenter
                                color: getStatusColor(modelData.status)
                            }
                        }
                        
                        // 底部分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.borderColor
                            anchors.bottom: parent.bottom
                        }
                    }
                }
                
                // 群组列表
                ListView {
                    id: groupsListView
                    clip: true
                    model: WebSocketManager.groups
                    
                    delegate: Rectangle {
                        width: groupsListView.width
                        height: 70
                        color: mouseArea.containsMouse ? Theme.highlightColor : "transparent"
                        
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                // 打开聊天页面
                                mainStackView.replace("ChatPage.qml")
                                var chatPage = mainStackView.currentItem
                                chatPage.currentChat = modelData
                                chatPage.isGroupChat = true
                                WebSocketManager.loadChatHistory(modelData.id, true)
                            }
                        }
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            spacing: 15
                            
                            // 群头像
                            Image {
                                source: modelData.avatar || "qrc:/assets/group_avatar.png"
                                width: 50
                                height: 50
                                Layout.alignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectCrop
                                layer.enabled: true
                                layer.effect: OpacityMask {
                                    maskSource: Rectangle {
                                        width: 50
                                        height: 50
                                        radius: 25
                                    }
                                }
                            }
                            
                            // 群信息
                            Column {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 5
                                
                                Text {
                                    text: modelData.name
                                    font.pixelSize: 16
                                    color: Theme.primaryTextColor
                                }
                                
                                Text {
                                    text: modelData.memberCount + " 成员"
                                    font.pixelSize: 12
                                    color: Theme.secondaryTextColor
                                }
                            }
                        }
                        
                        // 底部分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.borderColor
                            anchors.bottom: parent.bottom
                        }
                    }
                    
                    footer: Item {
                        width: parent.width
                        height: 60
                        
                        Button {
                            anchors.centerIn: parent
                            text: "创建新群组"
                            onClicked: {
                                mainStackView.push("CreateGroupPage.qml")
                            }
                        }
                    }
                }
                
                // 新的朋友请求
                ListView {
                    id: friendRequestsListView
                    clip: true
                    model: WebSocketManager.friendRequests
                    
                    delegate: Rectangle {
                        width: friendRequestsListView.width
                        height: 90
                        color: "transparent"
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            spacing: 15
                            
                            // 头像
                            Image {
                                source: modelData.avatar || "qrc:/assets/default_avatar.png"
                                width: 50
                                height: 50
                                Layout.alignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectCrop
                                layer.enabled: true
                                layer.effect: OpacityMask {
                                    maskSource: Rectangle {
                                        width: 50
                                        height: 50
                                        radius: 25
                                    }
                                }
                            }
                            
                            // 请求信息
                            Column {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 5
                                
                                Text {
                                    text: modelData.username
                                    font.pixelSize: 16
                                    color: Theme.primaryTextColor
                                }
                                
                                Text {
                                    text: modelData.message || "请求添加您为好友"
                                    font.pixelSize: 14
                                    color: Theme.secondaryTextColor
                                    width: parent.width
                                    wrapMode: Text.Wrap
                                }
                            }
                            
                            // 操作按钮
                            Column {
                                spacing: 10
                                Layout.alignment: Qt.AlignVCenter
                                
                                Button {
                                    text: "接受"
                                    width: 80
                                    onClicked: {
                                        WebSocketManager.acceptFriendRequest(modelData.id)
                                    }
                                }
                                
                                Button {
                                    text: "拒绝"
                                    width: 80
                                    onClicked: {
                                        WebSocketManager.rejectFriendRequest(modelData.id)
                                    }
                                }
                            }
                        }
                        
                        // 底部分隔线
                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Theme.borderColor
                            anchors.bottom: parent.bottom
                        }
                    }
                }
            }
        }
    }
    
    // 添加联系人对话框
    Dialog {
        id: addContactDialog
        title: "添加联系人"
        width: 400
        height: 300
        anchors.centerIn: Overlay.overlay
        modal: true
        
        contentItem: ColumnLayout {
            spacing: 15
            
            TextField {
                id: searchUserField
                Layout.fillWidth: true
                placeholderText: "用户名或邮箱"
                font.pixelSize: 16
            }
            
            Button {
                text: "搜索"
                Layout.fillWidth: true
                onClicked: {
                    // 搜索用户
                    WebSocketManager.searchUsers(searchUserField.text)
                }
            }
            
            ListView {
                id: searchResultsView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: WebSocketManager.searchResults
                
                delegate: Rectangle {
                    width: searchResultsView.width
                    height: 60
                    color: mouseArea.containsMouse ? Theme.highlightColor : "transparent"
                    
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            // 显示添加好友对话框
                            addFriendRequestDialog.username = modelData.username
                            addFriendRequestDialog.userId = modelData.id
                            addFriendRequestDialog.open()
                        }
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10
                        
                        Image {
                            source: modelData.avatar || "qrc:/assets/default_avatar.png"
                            width: 40
                            height: 40
                            Layout.alignment: Qt.AlignVCenter
                            fillMode: Image.PreserveAspectCrop
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Rectangle {
                                    width: 40
                                    height: 40
                                    radius: 20
                                }
                            }
                        }
                        
                        Text {
                            text: modelData.username
                            font.pixelSize: 16
                            color: Theme.primaryTextColor
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
        
        standardButtons: Dialog.Close
    }
    
    // 发送好友请求对话框
    Dialog {
        id: addFriendRequestDialog
        title: "添加好友"
        width: 400
        height: 200
        anchors.centerIn: Overlay.overlay
        modal: true
        
        property string username: ""
        property int userId: 0
        
        contentItem: ColumnLayout {
            spacing: 15
            
            Label {
                text: "发送好友请求给：" + addFriendRequestDialog.username
                font.pixelSize: 16
                Layout.fillWidth: true
            }
            
            TextField {
                id: verificationMessageField
                Layout.fillWidth: true
                placeholderText: "验证消息"
                font.pixelSize: 14
                text: "我是" + WebSocketManager.userInfo.username
            }
        }
        
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        onAccepted: {
            // 发送好友请求
            WebSocketManager.sendFriendRequest(userId, verificationMessageField.text)
        }
    }
    
    // 状态帮助函数
    function getStatusText(status) {
        switch(status) {
            case "online": return "在线"
            case "away": return "离开"
            case "busy": return "忙碌"
            default: return "离线"
        }
    }
    
    function getStatusColor(status) {
        switch(status) {
            case "online": return "#4CAF50" // 绿色 - 在线
            case "away": return "#FFC107"   // 黄色 - 离开
            case "busy": return "#F44336"   // 红色 - 忙碌
            default: return "#9E9E9E"       // 灰色 - 离线
        }
    }
} 