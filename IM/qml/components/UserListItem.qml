import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../utils"

Rectangle {
    id: userListItem
    width: parent.width
    height: 70
    color: highlighted ? Theme.highlightColor : "transparent"
    
    property string username: ""
    property string avatar: ""
    property string lastMessage: ""
    property var timestamp: new Date()
    property int unreadCount: 0
    property string status: "offline" // online, offline, away, busy
    property bool isGroup: false
    property bool highlighted: false
    
    signal clicked()
    
    MouseArea {
        anchors.fill: parent
        onClicked: {
            userListItem.clicked()
        }
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        
        // 头像
        Item {
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
            Layout.alignment: Qt.AlignVCenter
            
            Image {
                id: avatarImage
                anchors.fill: parent
                Image {
                    width: 50
                    height: 50
                    source: "qrc:/assets/default_avatar.png"
                }
                fillMode: Image.PreserveAspectCrop
                layer.enabled: true
                layer.effect: OpacityMask {
                    maskSource: Rectangle {
                        width: avatarImage.width
                        height: avatarImage.height
                        radius: avatarImage.width / 2
                    }
                }
            }
            
            // 在线状态指示器
            Rectangle {
                width: 14
                height: 14
                radius: 7
                visible: !isGroup && status !== ""
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                color: {
                    switch(status) {
                        case "online": return "#4CAF50" // 绿色 - 在线
                        case "away": return "#FFC107"   // 黄色 - 离开
                        case "busy": return "#F44336"   // 红色 - 忙碌
                        default: return "#9E9E9E"       // 灰色 - 离线
                    }
                }
                border.color: Theme.backgroundColor
                border.width: 2
            }
        }
        
        // 用户信息
        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 5
            
            // 用户名
            Text {
                text: username
                font.pixelSize: 16
                font.bold: true
                color: Theme.primaryTextColor
                elide: Text.ElideRight
                width: parent.width
            }
            
            // 最后一条消息
            Text {
                visible: lastMessage !== ""
                text: lastMessage
                font.pixelSize: 14
                color: Theme.secondaryTextColor
                elide: Text.ElideRight
                width: parent.width
            }
        }
        
        // 右侧信息
        Column {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.preferredWidth: 60
            spacing: 5
            
            // 时间戳
            Text {
                text: formatTime(timestamp)
                font.pixelSize: 12
                color: Theme.timestampColor
                anchors.right: parent.right
            }
            
            // 未读消息数
            Rectangle {
                visible: unreadCount > 0
                width: unreadCount > 9 ? 28 : 20
                height: 20
                radius: 10
                color: Theme.accentColor
                anchors.right: parent.right
                
                Text {
                    anchors.centerIn: parent
                    text: unreadCount > 99 ? "99+" : unreadCount
                    font.pixelSize: 12
                    color: "white"
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
    
    function formatTime(timestamp) {
        var now = new Date()
        var date = new Date(timestamp)
        
        // 如果是今天的消息，显示时间
        if (now.toDateString() === date.toDateString()) {
            var hours = date.getHours()
            var minutes = date.getMinutes()
            return (hours < 10 ? "0" : "") + hours + ":" + (minutes < 10 ? "0" : "") + minutes
        }
        
        // 如果是昨天的消息
        var yesterday = new Date(now)
        yesterday.setDate(now.getDate() - 1)
        if (yesterday.toDateString() === date.toDateString()) {
            return "昨天"
        }
        
        // 如果是本周的消息，显示星期几
        var weekDays = ["日", "一", "二", "三", "四", "五", "六"]
        if (now - date < 7 * 24 * 60 * 60 * 1000) {
            return "星期" + weekDays[date.getDay()]
        }
        
        // 其他情况显示日期
        return (date.getMonth() + 1) + "/" + date.getDate()
    }
}