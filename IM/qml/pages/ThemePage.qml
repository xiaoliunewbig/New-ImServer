import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../utils"

Page {
    id: themePage
    objectName: "themePage"
    
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            // 标题
            Label {
                text: "主题设置"
                font.pixelSize: 24
                font.bold: true
                color: Theme.primaryTextColor
            }
            
            // 主题选择
            GroupBox {
                title: "选择主题"
                Layout.fillWidth: true
                
                ColumnLayout {
                    spacing: 15
                    
                    RadioButton {
                        text: "浅色主题"
                        checked: Theme.currentTheme === "light"
                        onCheckedChanged: {
                            if (checked) {
                                Theme.setTheme("light")
                            }
                        }
                    }
                    
                    RadioButton {
                        text: "深色主题"
                        checked: Theme.currentTheme === "dark"
                        onCheckedChanged: {
                            if (checked) {
                                Theme.setTheme("dark")
                            }
                        }
                    }
                    
                    RadioButton {
                        text: "自定义主题"
                        checked: Theme.currentTheme === "custom"
                        onCheckedChanged: {
                            if (checked) {
                                Theme.setTheme("custom")
                            }
                        }
                    }
                }
            }
            
            // 主题预览
            GroupBox {
                title: "预览"
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                
                Rectangle {
                    anchors.fill: parent
                    color: Theme.backgroundColor
                    
                    RowLayout {
                        anchors.fill: parent
                        spacing: 10
                        
                        // 侧边栏预览
                        Rectangle {
                            Layout.preferredWidth: 60
                            Layout.fillHeight: true
                            color: Theme.sidebarColor
                            
                            Column {
                                anchors.centerIn: parent
                                spacing: 15
                                
                                Rectangle {
                                    width: 30
                                    height: 30
                                    radius: 15
                                    color: Theme.primaryColor
                                }
                                
                                Rectangle {
                                    width: 30
                                    height: 30
                                    radius: 15
                                    color: Theme.primaryColor
                                    opacity: 0.5
                                }
                            }
                        }
                        
                        // 联系人列表预览
                        Rectangle {
                            Layout.preferredWidth: 150
                            Layout.fillHeight: true
                            color: Theme.secondaryBackgroundColor
                            border.color: Theme.borderColor
                            
                            Column {
                                anchors.centerIn: parent
                                spacing: 10
                                width: parent.width - 20
                                
                                Rectangle {
                                    width: parent.width
                                    height: 20
                                    color: Theme.inputBackgroundColor
                                    radius: 4
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: Theme.highlightColor
                                    radius: 4
                                }
                                
                                Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: "transparent"
                                    radius: 4
                                    border.color: Theme.borderColor
                                }
                            }
                        }
                        
                        // 聊天区域预览
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: Theme.backgroundColor
                            
                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 10
                                
                                // 标题栏
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 40
                                    color: Theme.headerColor
                                }
                                
                                // 消息区域
                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    
                                    // 接收消息
                                    Rectangle {
                                        width: 120
                                        height: 30
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10
                                        anchors.top: parent.top
                                        anchors.topMargin: 10
                                        color: Theme.bubbleColor
                                        radius: 10
                                    }
                                    
                                    // 发送消息
                                    Rectangle {
                                        width: 120
                                        height: 30
                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.top: parent.top
                                        anchors.topMargin: 50
                                        color: Theme.primaryColor
                                        radius: 10
                                    }
                                }
                                
                                // 输入区域
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 40
                                    color: Theme.inputAreaColor
                                }
                            }
                        }
                    }
                }
            }
            
            // 自定义颜色
            GroupBox {
                title: "自定义颜色"
                Layout.fillWidth: true
                visible: Theme.currentTheme === "custom"
                
                GridLayout {
                    columns: 2
                    rowSpacing: 10
                    columnSpacing: 15
                    
                    Label { text: "主色调" }
                    ColorPicker {
                        onColorSelected: {
                            // 自定义主色调
                        }
                    }
                    
                    Label { text: "强调色" }
                    ColorPicker {
                        onColorSelected: {
                            // 自定义强调色
                        }
                    }
                    
                    Label { text: "背景色" }
                    ColorPicker {
                        onColorSelected: {
                            // 自定义背景色
                        }
                    }
                }
            }
            
            Item { Layout.fillHeight: true }
            
            // 应用按钮
            Button {
                text: "应用主题"
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    // 保存主题设置
                    localStorage.setItem("theme", Theme.currentTheme)
                }
            }
        }
    }
} 