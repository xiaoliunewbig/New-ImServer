import QtQuick 6.0
import QtQuick.Controls 6.0

Column {
    spacing: Theme.spacingMedium

    Button {
        id: selectFileButton
        text: "选择文件"
        width: parent.width
        onClicked: fileDialog.open()
    }

    Text {
        id: fileNameLabel
        text: "未选择文件"
        color: Theme.secondaryTextColor
    }

    Button {
        id: uploadButton
        text: "上传文件"
        width: parent.width
        enabled: false
        onClicked: {
            WebSocketManager.sendMessage({
                type: "uploadFile",
                data: {
                    fileUrl: fileUrl
                }
            })
        }
    }

    FileDialog {
        id: fileDialog
        onFileUrlChanged: {
            fileNameLabel.text = fileUrl.toString().split("/").pop()
            uploadButton.enabled = true
        }
    }
}

WebSocketManager.onMessageReceived: function(message) {
    if (message.type === "uploadFile") {
        if (message.data.success) {
            console.log("文件上传成功");
        } else {
            console.error("文件上传失败: " + message.data.message);
            showToast("文件上传失败: " + message.data.message);
        }
    }
}