pragma Singleton
import QtQuick 6.0

QtObject {
    id: theme

    // 当前主题
    property string currentTheme: "light" // light, dark, custom

    // 颜色
    property color primaryColor: currentTheme === "dark" ? "#2979FF" : "#1976D2"
    property color accentColor: currentTheme === "dark" ? "#FF4081" : "#E91E63"
    property color backgroundColor: currentTheme === "dark" ? "#121212" : "#F5F5F5"
    property color secondaryBackgroundColor: currentTheme === "dark" ? "#1E1E1E" : "#FFFFFF"
    property color headerColor: currentTheme === "dark" ? "#1E1E1E" : "#FFFFFF"
    property color sidebarColor: currentTheme === "dark" ? "#272727" : "#FFFFFF"
    property color inputAreaColor: currentTheme === "dark" ? "#1E1E1E" : "#FFFFFF"
    property color bubbleColor: currentTheme === "dark" ? "#424242" : "#E0E0E0"

    property color primaryTextColor: currentTheme === "dark" ? "#FFFFFF" : "#212121"
    property color secondaryTextColor: currentTheme === "dark" ? "#B0B0B0" : "#757575"
    property color linkColor: currentTheme === "dark" ? "#64B5F6" : "#1976D2"
    property color timestampColor: currentTheme === "dark" ? "#9E9E9E" : "#9E9E9E"
    property color inputTextColor: currentTheme === "dark" ? "#FFFFFF" : "#212121"

    property color highlightColor: currentTheme === "dark" ? Qt.rgba(255, 255, 255, 0.08) : Qt.rgba(0, 0, 0, 0.05)
    property color borderColor: currentTheme === "dark" ? "#424242" : "#E0E0E0"
    property color inputBackgroundColor: currentTheme === "dark" ? "#333333" : "#F5F5F5"

    // 动画相关
    property int animationDuration: 300
    property int shortAnimationDuration: 150

    // 字体
    property int fontSizeSmall: 12
    property int fontSizeMedium: 14
    property int fontSizeLarge: 16
    property int fontSizeXLarge: 18

    // 尺寸
    property int spacingSmall: 4
    property int spacingMedium: 8
    property int spacingLarge: 12
    property int spacingXLarge: 16

    // 圆角
    property int borderRadiusSmall: 4
    property int borderRadiusMedium: 8
    property int borderRadiusLarge: 12

    // 阴影
    property var shadow: currentTheme === "dark" ? {
        color: Qt.rgba(0, 0, 0, 0.5),
        radius: 8,
        offsetX: 0,
        offsetY: 2
    } : {
        color: Qt.rgba(0, 0, 0, 0.15),
        radius: 4,
        offsetX: 0,
        offsetY: 1
    }

    // 设置主题
    function setTheme(theme) {
        currentTheme = theme
    }

    // 获取颜色对比度
    function getContrastColor(baseColor) {
        var luminance = (0.299 * baseColor.r + 0.587 * baseColor.g + 0.114 * baseColor.b)
        return luminance > 0.5 ? "#000000" : "#FFFFFF"
    }

    // 获取半透明颜色
    function getTransparentColor(baseColor, opacity) {
        return Qt.rgba(baseColor.r, baseColor.g, baseColor.b, opacity)
    }

    // 获取渐变色
    function getGradientColor(startColor, endColor) {
        return Qt.rgba(
            (startColor.r + endColor.r) / 2,
            (startColor.g + endColor.g) / 2,
            (startColor.b + endColor.b) / 2,
            1
        )
    }
}