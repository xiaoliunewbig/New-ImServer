import QtQuick 6.0
import QtQuick.Controls 6.0
import QtQuick.Layouts 6.0
import QtQuick.Dialogs

Rectangle {
    id: colorPickerButton
    width: 100
    height: 30
    radius: 4
    border.color: "black"
    border.width: 1
    
    property color selectedColor: "blue"
    
    signal colorSelected(color color)
    
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 3
        color: selectedColor
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: {
            colorDialog.open()
        }
    }
    
    ColorDialog {
        id: colorDialog
        title: "选择颜色"
        onAccepted: {
            selectedColor = colorDialog.color
            colorSelected(selectedColor)
        }
    }
} 