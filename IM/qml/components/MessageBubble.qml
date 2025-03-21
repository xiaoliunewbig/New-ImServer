import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../utils"

Item {
    id: messageBubble
    width: parent.width
    height: contentColumn.height + 20
    
    property string message: ""
    property bool isOwnMessage: false
    property string senderName: ""
    property string avatarUrl: ""
    property var timestamp: new Date()
    property string messageType: "text" // text, image, file
    property string fileUrl: ""
    property int fileSize: 0
    property string imageUrl: ""
    
    // 动画相关属性
    property real bubbleScale: 1.0
    property real bubbleOpacity: 1.0
    property real bubbleXOffset: isOwnMessage ? 50 : -50

    states: [
        State {
            name: "entering"
            PropertyChanges { target: messageBubble; bubbleScale: 1.0; bubbleOpacity: 1.0; bubbleXOffset: 0 }
        },
        State {
            name: "exiting"
            PropertyChanges { target: messageBubble; bubbleScale: 0.9; bubbleOpacity: 0; bubbleXOffset: isOwnMessage ? 50 : -50 }
        }
    ]

    transitions: [
        Transition {
            from: ""; to: "entering"
            ParallelAnimation {
                NumberAnimation { property: "bubbleScale"; from: 0.8; to: 1.0; duration: 300; easing.type: Easing.OutBack }
                NumberAnimation { property: "bubbleOpacity"; from: 0; to: 1; duration: 300; easing.type: Easing.OutQuad }
                NumberAnimation { property: "bubbleXOffset"; duration: 300; easing.type: Easing.OutCubic }
            }
        },
        Transition {
            from: "*"; to: "exiting"
            ParallelAnimation {
                NumberAnimation { property: "bubbleScale"; duration: 200; easing.type: Easing.InQuad }
                NumberAnimation { property: "bubbleOpacity"; duration: 200; easing.type: Easing.InQuad }
                NumberAnimation { property: "bubbleXOffset"; duration: 200; easing.type: Easing.InCubic }
            }
        }
    ]

    transform: [
        Scale {
            origin.x: isOwnMessage ? width : 0
            origin.y: 0
            xScale: bubbleScale
            yScale: bubbleScale
        },
        Translate {
            x: bubbleXOffset
        }
    ]

    opacity: bubbleOpacity

    Component.onCompleted: {
        state = "entering"
    }

    Component.onDestruction: {
        state = "exiting"
    }

    anchors {
        left: parent.left
        right: parent.right
        margins: 10
    }
    
    RowLayout {
        width: parent.width
        anchors.top: parent.top
        spacing: 10
        
        // 头像部分 - 如果是自己的消息，则在右边显示头像
        Item {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignTop
            visible: !isOwnMessage
            
            Image {
                id: avatar
                anchors.fill: parent
                source: avatarUrl || "qrc:/assets/default_avatar.png"
                fillMode: Image.PreserveAspectCrop
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: avatar.width
                        height: avatar.height
                        radius: avatar.width / 2
                    }
                }
            }
        }
        
        // 消息内容部分
        Column {
            id: contentColumn
            Layout.fillWidth: true
            spacing: 5
            Layout.alignment: isOwnMessage ? Qt.AlignRight : Qt.AlignLeft
            
            // 发送者名称
            Text {
                text: senderName
                font.pixelSize: 12
                color: Theme.secondaryTextColor
                visible: !isOwnMessage
            }
            
            // 消息气泡
            Rectangle {
                width: Math.min(messageContent.implicitWidth + 24, parent.width * 0.75)
                height: messageContent.height + 20
                radius: 10
                color: isOwnMessage ? Theme.primaryColor : Theme.bubbleColor
                
                // 消息内容 - 根据消息类型显示不同内容
                Item {
                    id: messageContent
                    anchors.centerIn: parent
                    width: childrenRect.width
                    height: childrenRect.height
                    
                    // 文本消息
                    Text {
                        id: textMessage
                        visible: messageType === "text"
                        text: message
                        width: Math.min(implicitWidth, parent.parent.width - 24)
                        wrapMode: Text.Wrap
                        font.pixelSize: 14
                        color: isOwnMessage ? "white" : Theme.primaryTextColor
                    }
                    
                    // 图片消息
                    Image {
                        id: imageMessage
                        visible: messageType === "image"
                        source: imageUrl
                        width: 200
                        height: 150
                        fillMode: Image.PreserveAspectFit
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                var component = Qt.createComponent("../components/ImageViewer.qml")
                                if (component.status === Component.Ready) {
                                    var imageViewer = component.createObject(applicationWindow, {imageSource: imageUrl})
                                    imageViewer.open()
                                }
                            }
                        }
                    }
                    
                    // 文件消息
                    Rectangle {
                        id: fileMessage
                        visible: messageType === "file"
                        width: 240
                        height: 70
                        color: "transparent"
                        
                        RowLayout {
                            anchors.fill: parent
                            spacing: 10
                            
                            Image {
                                source: "qrc:/assets/file_icon.png"
                                width: 32
                                height: 32
                                Layout.alignment: Qt.AlignVCenter
                            }
                            
                            Column {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 5
                                
                                Text {
                                    text: message // 文件名
                                    font.pixelSize: 14
                                    elide: Text.ElideMiddle
                                    width: parent.width
                                    color: isOwnMessage ? "white" : Theme.primaryTextColor
                                }
                                
                                Text {
                                    text: formatFileSize(fileSize)
                                    font.pixelSize: 12
                                    color: isOwnMessage ? Qt.rgba(1, 1, 1, 0.7) : Theme.secondaryTextColor
                                }
                            }
                            
                            Button {
                                text: "下载"
                                Layout.alignment: Qt.AlignVCenter
                                onClicked: {
                                    WebSocketManager.downloadFile(fileUrl, message)
                                }
                            }
                        }
                    }
                }
            }
            
            // 时间戳
            Text {
                text: formatTime(timestamp)
                font.pixelSize: 10
                color: Theme.timestampColor
                anchors.right: isOwnMessage ? parent.right : undefined
            }
        }
        
        // 自己的消息在左侧显示头像
        Item {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignTop
            visible: isOwnMessage
            
            Image {
                id: ownAvatar
                anchors.fill: parent
                source: WebSocketManager.userInfo.avatar || "qrc:/assets/default_avatar.png"
                fillMode: Image.PreserveAspectCrop
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: ownAvatar.width
                        height: ownAvatar.height
                        radius: ownAvatar.width / 2
                    }
                }
            }
        }
    }
    
    // 辅助函数
    function formatTime(timestamp) {
        var date = new Date(timestamp)
        var hours = date.getHours()
        var minutes = date.getMinutes()
        return (hours < 10 ? "0" : "") + hours + ":" + (minutes < 10 ? "0" : "") + minutes
    }
    
    function formatFileSize(bytes) {
        if (bytes < 1024) {
            return bytes + " B"
        } else if (bytes < 1024 * 1024) {
            return (bytes / 1024).toFixed(1) + " KB"
        } else if (bytes < 1024 * 1024 * 1024) {
            return (bytes / (1024 * 1024)).toFixed(1) + " MB"
        } else {
            return (bytes / (1024 * 1024 * 1024)).toFixed(1) + " GB"
        }
    }
}