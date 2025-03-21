import QtQuick 6.0
import QtQuick.Window 6.0
import QtQuick.Controls 6.0

ApplicationWindow {
    id: applicationWindow
    width: 1280
    height: 800
    visible: true
    title: "IM Chat Client"

    // 使用StackView管理主要页面
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: LoginWindow {}
    }

    // 全局函数：导航到登录页面
    function navigateToLogin() {
        stackView.clear();
        stackView.push("LoginWindow.qml");
        WebSocketManager.disconnect(); // 断开 WebSocket 连接
    }

    // 全局函数：导航到主界面
    function navigateToMain() {
        stackView.clear();
        stackView.push("MainWindow.qml");
        WebSocketManager.connect(); // 连接 WebSocket
    }

    // 设置初始主题
    Component.onCompleted: {
        Theme.setTheme("light")
    }

    // 全局属性：当前用户信息
    property var currentUser: WebSocketManager.userInfo || null;

    // 全局函数：更新用户信息
    function updateUserInfo(userInfo) {
        currentUser = userInfo;
        WebSocketManager.userInfo = userInfo;
    }

    // 全局函数：显示错误提示
    function showError(message) {
        errorToast.text = message;
        errorToast.visible = true;
        errorTimer.start();
    }

    // 全局组件：错误提示
    Toast {
        id: errorToast
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
    }

    Timer {
        id: errorTimer
        interval: 3000
        onTriggered: errorToast.visible = false
    }

    // 全局函数：切换主题
    function switchTheme(theme) {
        Theme.setTheme(theme);
        WebSocketManager.sendMessage({
            type: "updateTheme",
            data: {
                theme: theme
            }
        });
    }
}