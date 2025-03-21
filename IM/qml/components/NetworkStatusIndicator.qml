import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    width: 20
    height: 20
    radius: 10
    color: {
        switch (status) {
        case "connected": return "green"
        case "connecting": return "yellow"
        case "disconnected": return "red"
        default: return "gray"
        }
    }

    property string status: "disconnected"

    Behavior on color {
        ColorAnimation { duration: 300 }
    }

    ToolTip {
        visible: parent.hovered
        text: {
            switch (status) {
            case "connected": return "已连接"
            case "connecting": return "连接中"
            case "disconnected": return "已断开"
            default: return "未知状态"
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
    }
}