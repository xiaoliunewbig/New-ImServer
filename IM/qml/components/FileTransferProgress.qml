import QtQuick 6.0
import QtQuick.Controls 6.0

Rectangle {
    width: 300
    height: 100
    radius: 8
    color: Theme.secondaryBackgroundColor

    property string fileName: ""
    property int progress: 0
    property string status: "uploading" // uploading, downloading, paused, failed

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: fileName
            font.pixelSize: 14
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        ProgressBar {
            value: progress / 100
            Layout.fillWidth: true

            Behavior on value {
                NumberAnimation { duration: 300 }
            }
        }

        RowLayout {
            spacing: 10
            Layout.alignment: Qt.AlignRight

            Button {
                text: "暂停"
                visible: status === "uploading" || status === "downloading"
                onClicked: {
                    status = status === "paused" ? "uploading" : "paused"
                }
            }

            Button {
                text: "取消"
                onClicked: {
                    status = "failed"
                }
            }

            Behavior on color {
                ColorAnimation { duration: 300 }
            }
        }
    }
}