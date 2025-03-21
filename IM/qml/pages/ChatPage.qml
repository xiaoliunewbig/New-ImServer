import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Dialogs
import "../components"
import "../utils"

Page {
    id: chatPage
    objectName: "chatPage"

    property var currentChat: null // 当前聊天对象 (用户或群组)
    property bool isGroupChat: false // 是否为群聊

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 聊天列表区域
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: Theme.secondaryBackgroundColor
            border.width: 1
            border.color: Theme.borderColor

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 搜索框
                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    color: "transparent"

                    TextField {
                        id: searchField
                        anchors.centerIn: parent
                        width: parent.width - 40
                        height: 40
                        placeholderText: "搜索"
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

                // 标签栏
                TabBar {
                    id: chatTabBar
                    Layout.fillWidth: true
                    height: 50
                    
                    TabButton {
                        text: "最近聊天"
                        width: implicitWidth
                    }
                    
                    TabButton {
                        text: "联系人"
                        width: implicitWidth
                    }
                    
                    TabButton {
                        text: "群组"
                        width: implicitWidth
                    }
                }

                // 聊天列表
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: chatTabBar.currentIndex

                    // 最近聊天
                    ListView {
                        id: recentChatList
                        clip: true
                        model: WebSocketManager.recentChats
                        delegate: UserListItem {
                            width: recentChatList.width
                            height: 70
                            username: modelData.name
                            avatar: modelData.avatar
                            lastMessage: modelData.lastMessage
                            timestamp: modelData.timestamp
                            unreadCount: modelData.unreadCount
                            isGroup: modelData.isGroup
                            onClicked: {
                                chatPage.currentChat = modelData
                                chatPage.isGroupChat = modelData.isGroup
                                WebSocketManager.loadChatHistory(modelData.id, modelData.isGroup)
                            }
                            highlighted: chatPage.currentChat && chatPage.currentChat.id === modelData.id
                        }
                    }

                    // 联系人
                    ListView {
                        id: contactsList
                        clip: true
                        model: WebSocketManager.contacts
                        delegate: UserListItem {
                            width: contactsList.width
                            height: 70
                            username: modelData.username
                            avatar: modelData.avatar
                            status: modelData.status
                            onClicked: {
                                chatPage.currentChat = modelData
                                chatPage.isGroupChat = false
                                WebSocketManager.loadChatHistory(modelData.id, false)
                            }
                            highlighted: chatPage.currentChat && !chatPage.isGroupChat && chatPage.currentChat.id === modelData.id
                        }
                    }

                    // 群组
                    ListView {
                        id: groupsList
                        clip: true
                        model: WebSocketManager.groups
                        delegate: GroupListItem {
                            width: groupsList.width
                            height: 70
                            groupName: modelData.name
                            groupAvatar: modelData.avatar
                            memberCount: modelData.memberCount
                            onClicked: {
                                chatPage.currentChat = modelData
                                chatPage.isGroupChat = true
                                WebSocketManager.loadChatHistory(modelData.id, true)
                            }
                            highlighted: chatPage.currentChat && chatPage.isGroupChat && chatPage.currentChat.id === modelData.id
                        }

                        footer: Button {
                            width: parent.width
                            height: 50
                            text: "创建新群组"
                            onClicked: {
                                mainStackView.push("CreateGroupPage.qml")
                            }
                        }
                    }
                }
            }
        }

        // 聊天内容区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.backgroundColor
            visible: currentChat !== null

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 聊天标题栏
                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    color: Theme.headerColor

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20

                        // 头像
                        Image {
                            id: chatAvatar
                            source: currentChat ? currentChat.avatar || "qrc:/assets/default_avatar.png" : ""
                            width: 40
                            height: 40
                            Layout.alignment: Qt.AlignVCenter
                            fillMode: Image.PreserveAspectCrop
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Rectangle {
                                    width: chatAvatar.width
                                    height: chatAvatar.height
                                    radius: chatAvatar.width / 2
                                }
                            }
                        }

                        // 聊天名称
                        Column {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            Text {
                                text: currentChat ? currentChat.name || currentChat.username : ""
                                font.pixelSize: 16
                                font.bold: true
                                color: Theme.primaryTextColor
                            }

                            Text {
                                visible: isGroupChat
                                text: currentChat && isGroupChat ? currentChat.memberCount + " 成员" : ""
                                font.pixelSize: 12
                                color: Theme.secondaryTextColor
                            }
                        }

                        // 操作按钮
                        Row {
                            spacing: 15
                            Layout.alignment: Qt.AlignVCenter

                            Image {
                                source: "qrc:/assets/call_icon.png"
                                width: 24
                                height: 24
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        // 语音通话功能
                                    }
                                }
                            }

                            Image {
                                source: "qrc:/assets/video_icon.png"
                                width: 24
                                height: 24
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        // 视频通话功能
                                    }
                                }
                            }

                            Image {
                                source: "qrc:/assets/more_icon.png"
                                width: 24
                                height: 24
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        // 更多选项
                                        chatMenu.open()
                                    }
                                }

                                Menu {
                                    id: chatMenu
                                    MenuItem {
                                        text: "查看资料"
                                        onTriggered: {
                                            // 打开资料页
                                        }
                                    }
                                    MenuItem {
                                        text: isGroupChat ? "查看群成员" : "添加好友"
                                        onTriggered: {
                                            // 查看群成员或添加好友
                                        }
                                    }
                                    MenuItem {
                                        text: isGroupChat ? "退出群组" : "删除好友"
                                        onTriggered: {
                                            // 退出群组或删除好友
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 聊天内容区域
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "transparent"

                    ScrollView {
                        id: messageScrollView
                        anchors.fill: parent
                        anchors.bottomMargin: 10
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded

                        ListView {
                            id: messageListView
                            spacing: 15
                            model: WebSocketManager.currentChatMessages
                            delegate: MessageBubble {
                                width: messageListView.width
                                message: modelData.content
                                isOwnMessage: modelData.fromUserId === WebSocketManager.userInfo.id
                                senderName: modelData.senderName
                                avatarUrl: modelData.senderAvatar
                                timestamp: modelData.timestamp
                                messageType: modelData.messageType
                                fileUrl: modelData.fileUrl
                                fileSize: modelData.fileSize
                                imageUrl: modelData.imageUrl
                            }

                            // 新消息自动滚动到底部
                            onCountChanged: {
                                positionViewAtEnd()
                            }
                        }
                    }
                }

                // 输入区域
                Rectangle {
                    Layout.fillWidth: true
                    height: 150
                    color: Theme.inputAreaColor

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        // 消息工具栏
                        Row {
                            spacing: 10
                            Layout.fillWidth: true
                            height: 30

                            // 表情按钮
                            RoundButton {
                                id: emojiButton
                                width: 30
                                height: 30
                                icon.source: "qrc:/assets/emoji_icon.png"
                                onClicked: {
                                    emojiPicker.visible = !emojiPicker.visible
                                }
                            }

                            // 文件按钮
                            RoundButton {
                                id: fileButton
                                width: 30
                                height: 30
                                icon.source: "qrc:/assets/file_icon.png"
                                onClicked: {
                                    fileDialog.open()
                                }
                            }

                            // 图片按钮
                            RoundButton {
                                id: imageButton
                                width: 30
                                height: 30
                                icon.source: "qrc:/assets/image_icon.png"
                                onClicked: {
                                    imageDialog.open()
                                }
                            }

                            // 表情选择器
                            EmojiPicker {
                                id: emojiPicker
                                visible: false
                                anchors.bottom: emojiButton.top
                                anchors.left: emojiButton.left
                                width: 300
                                height: 200
                                onEmojiSelected: function(emoji) {
                                    messageInput.insert(messageInput.cursorPosition, emoji)
                                    visible = false
                                }
                            }
                        }

                        // 消息输入框
                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            TextArea {
                                id: messageInput
                                placeholderText: "输入消息..."
                                wrapMode: TextArea.Wrap
                                selectByMouse: true
                                background: Rectangle {
                                    color: "transparent"
                                }
                                Keys.onReturnPressed: function(event) {
                                    if (event.modifiers & Qt.ControlModifier) {
                                        insert(cursorPosition, "\n")
                                    } else {
                                        sendButton.clicked()
                                        event.accepted = true
                                    }
                                }
                            }
                        }

                        // 发送按钮行
                        RowLayout {
                            Layout.fillWidth: true
                            height: 40
                            
                            Item { Layout.fillWidth: true }
                            
                            Button {
                                id: sendButton
                                text: "发送"
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 36
                                enabled: messageInput.text.trim() !== "" && currentChat !== null
                                
                                background: Rectangle {
                                    color: sendButton.enabled ? (sendButton.down ? Qt.darker(Theme.primaryColor, 1.1) : Theme.primaryColor) : Qt.lighter(Theme.primaryColor, 1.5)
                                    radius: 4
                                }
                                
                                contentItem: Text {
                                    text: sendButton.text
                                    color: "white"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                onClicked: {
                                    if (messageInput.text.trim() !== "" && currentChat !== null) {
                                        WebSocketManager.sendMessage(
                                            currentChat.id, 
                                            messageInput.text, 
                                            isGroupChat, 
                                            "text"
                                        )
                                        messageInput.clear()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 提示选择聊天
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.backgroundColor
            visible: currentChat === null

            Column {
                anchors.centerIn: parent
                spacing: 20

                Image {
                    source: "qrc:/assets/chat_placeholder.png"
                    width: 120
                    height: 120
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: "选择一个联系人或群组开始聊天"
                    font.pixelSize: 16
                    color: Theme.secondaryTextColor
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "选择文件"
        nameFilters: ["所有文件 (*)"]
        onAccepted: {
            if (currentChat !== null) {
                WebSocketManager.sendFile(
                    currentChat.id, 
                    fileDialog.selectedFile, 
                    isGroupChat
                )
            }
        }
    }

    // 图片选择对话框
    FileDialog {
        id: imageDialog
        title: "选择图片"
        nameFilters: ["图片文件 (*.jpg *.jpeg *.png *.gif)"]
        onAccepted: {
            if (currentChat !== null) {
                WebSocketManager.sendImage(
                    currentChat.id, 
                    imageDialog.selectedFile, 
                    isGroupChat
                )
            }
        }
    }
}