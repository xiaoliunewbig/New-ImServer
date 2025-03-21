import QtQuick 6.0
import QtQuick.Controls 6.0

Row {
    spacing: Theme.spacingMedium

    // 浅色主题按钮
    Button {
        text: "浅色主题"
        onClicked: {
            Theme.setTheme("light")
            WebSocketManager.sendMessage({
                type: "updateTheme",
                data: {
                    theme: "light"
                }
            })
            showToast("已切换至浅色主题")
        }
    }

    // 深色主题按钮
    Button {
        text: "深色主题"
        onClicked: {
            Theme.setTheme("dark")
            WebSocketManager.sendMessage({
                type: "updateTheme",
                data: {
                    theme: "dark"
                }
            })
            showToast("已切换至深色主题")
        }
    }

    // 显示提示信息
    function showToast(message) {
        toast.text = message
        toast.visible = true
        toastTimer.start()
    }

    // 提示信息组件
    Text {
        id: toast
        visible: false
        color: Theme.primaryTextColor
        anchors.centerIn: parent
    }

    // 提示信息定时器
    Timer {
        id: toastTimer
        interval: 2000
        onTriggered: toast.visible = false
    }
}