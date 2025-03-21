import QtQuick 6.0

QtObject {
    function formatTimestamp(timestamp) {
        var date = new Date(timestamp)
        var now = new Date()
        if (date.toDateString() === now.toDateString()) {
            return date.toLocaleTimeString(Qt.locale(), "hh:mm")
        } else if (date.getFullYear() === now.getFullYear()) {
            return date.toLocaleDateString(Qt.locale(), "MM/dd hh:mm")
        } else {
            return date.toLocaleDateString(Qt.locale(), "yyyy/MM/dd hh:mm")
        }
    }
}