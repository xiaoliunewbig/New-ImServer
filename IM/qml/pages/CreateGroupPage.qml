import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import "../components"
import "../utils"

Page {
    id: createGroupPage
    objectName: "createGroupPage"
    title: "创建群组"
    
    property var selectedContacts: []
    
    Rectangle {
        anchors.fill: parent
        color: Theme.backgroundColor
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20
            
            // 标题
            Label {
                text: "创建新群组"
                font.pixelSize: 24
                font.bold: true
                color: Theme.primaryTextColor
            }
            
            // 群组信息
            TextField {
                id: groupNameField
                Layout.fillWidth: true
                placeholderText: "群组名称"
                font.pixelSize: 16
            }
            
            // 搜索联系人
            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "搜索联系人"
                font.pixelSize: 16
                leftPadding: 40
                
                Image {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 16
                    height: 16
                    source: "qrc:/assets/search_icon.png"
                }
                
                onTextChanged: {
                    // 过滤联系人列表
                    // 实际应用中应该在这里筛选联系人
                }
            }
            
            // 已选联系人
            Label {
                text: "已选择: " + selectedContacts.length + " 人"
                font.pixelSize: 14
                color: Theme.secondaryTextColor
                visible: selectedContacts.length > 0
            }
            
            // 已选联系人流式布局
            Flow {
                Layout.fillWidth: true
                spacing: 10
                visible: selectedContacts.length > 0
                
                Repeater {
                    model: selectedContacts
                    
                    Rectangle {
                        width: contactName.width + 40
                        height: 36
                        radius: 18
                        color: Theme.primaryColor
                        
                        Text {
                            id: contactName
                            anchors.centerIn: parent
                            text: modelData.username
                            color: "white"
                            font.pixelSize: 14
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                // 移除联系人
                                var index = selectedContacts.indexOf(modelData)
                                if (index > -1) {
                                    selectedContacts.splice(index, 1)
                                    selectedContacts = [...selectedContacts] // 强制更新
                                }
                            }
                        }
                    }
                }
            }
            
            // 联系人列表
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.secondaryBackgroundColor
                radius: 10
                border.color: Theme.borderColor
                
                ListView {
                    id: contactsList
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true
                    model: WebSocketManager.contacts
                    
                    delegate: Rectangle {
                        width: contactsList.width
                        height: 60
                        color: "transparent"
                        
                        RowLayout {
                            anchors.fill: parent
                            spacing: 10
                            
                            // 复选框
                            CheckBox {
                                checked: isSelected(modelData)
                                onCheckedChanged: {
                                    if (checked) {
                                        addContact(modelData)
                                    } else {
                                        removeContact(modelData)
                                    }
                                }
                            }
                            
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
                            
                            // 用户名
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
            
            // 按钮行
            RowLayout {
                Layout.fillWidth: true
                spacing: 20
                
                Button {
                    text: "取消"
                    Layout.preferredWidth: 100
                    onClicked: {
                        mainStackView.pop()
                    }
                }
                
                Item { Layout.fillWidth: true }
                
                Button {
                    text: "创建群组"
                    Layout.preferredWidth: 100
                    enabled: groupNameField.text.trim() !== "" && selectedContacts.length > 0
                    
                    background: Rectangle {
                        color: parent.enabled ? (parent.down ? Qt.darker(Theme.primaryColor, 1.1) : Theme.primaryColor) : Qt.lighter(Theme.primaryColor, 1.5)
                        radius: 4
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    onClicked: {
                        if (createGroup()) {
                            mainStackView.pop()
                        }
                    }
                }
            }
        }
    }
    
    // 判断是否已选
    function isSelected(contact) {
        return selectedContacts.some(c => c.id === contact.id)
    }
    
    // 添加联系人
    function addContact(contact) {
        if (!isSelected(contact)) {
            selectedContacts.push(contact)
            selectedContacts = [...selectedContacts] // 强制更新
        }
    }
    
    // 移除联系人
    function removeContact(contact) {
        var index = selectedContacts.findIndex(c => c.id === contact.id)
        if (index > -1) {
            selectedContacts.splice(index, 1)
            selectedContacts = [...selectedContacts] // 强制更新
        }
    }
    
// 创建群组
function createGroup() {
    if (groupNameField.text.trim() === "" || selectedContacts.length === 0) {
        return false
    }
    
    // 模拟创建群组
    var memberIds = selectedContacts.map(c => c.id)
    
    // 调用后端API创建群组
    var success = WebSocketManager.createGroup(groupNameField.text, memberIds)
    
    return success
} 
}