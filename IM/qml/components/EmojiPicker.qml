import QtQuick 6.0
import QtQuick.Controls 6.0
import "../utils"

Rectangle {
    id: emojiPicker
    width: 300
    height: 200
    color: Theme.backgroundColor
    border.color: Theme.borderColor
    border.width: 1
    radius: 10
    
    signal emojiSelected(string emoji)
    
    // 表情符号数组
    property var emojis: [
        "😀", "😁", "😂", "🤣", "😃", "😄", "😅", "😆", 
        "😉", "😊", "😋", "😎", "😍", "😘", "😗", "😙",
        "😚", "🙂", "🤗", "🤔", "😐", "😑", "😶", "🙄",
        "😏", "😣", "😥", "😮", "🤐", "😯", "😪", "😫",
        "😴", "😌", "😛", "😜", "😝", "🤤", "😒", "😓",
        "😔", "😕", "🙃", "🤑", "😲", "🙁", "😖", "😞",
        "😟", "😤", "😢", "😭", "😦", "😧", "😨", "😩",
        "🤯", "😬", "😰", "😱", "😳", "🤪", "😵", "😡"
    ]
    
    // 分类标签
    TabBar {
        id: categoryTabs
        width: parent.width
        anchors.top: parent.top
        
        TabButton {
            text: "表情"
        }
        
        TabButton {
            text: "手势"
        }
        
        TabButton {
            text: "动物"
        }
    }
    
    // 表情网格
    GridView {
        id: emojiGrid
        anchors.top: categoryTabs.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        clip: true
        cellWidth: 40
        cellHeight: 40
        model: emojis
        
        delegate: Rectangle {
            width: 35
            height: 35
            color: mouseArea.containsMouse ? Theme.highlightColor : "transparent"
            radius: 4
            
            Text {
                anchors.centerIn: parent
                text: modelData
                font.pixelSize: 20
            }
            
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    emojiPicker.emojiSelected(modelData)
                }
            }
            Image {
                width: 24
                height: 24
                source: "qrc:/assets/emoji_icon.png"
            }
        }
    }
}