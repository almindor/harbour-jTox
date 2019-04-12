import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/common.js" as Common

Column {
    id: mainColumn
    anchors {
        left: parent.left
        right: parent.right
    }

    Row {
        spacing: Theme.paddingSmall
        layoutDirection: Common.isEventIncoming(event_type) ? Qt.RightToLeft : Qt.LeftToRight

        Image {
            id: paperclip
            source: "image://theme/icon-m-transfer?" + colorForEventMsg(event_type)
        }

        Text {
            width: mainColumn.width - paperclip.width - Theme.paddingSmall
            text: message
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.strikeout: Common.isFileCanceled(event_type)
            font.pixelSize: Theme.fontSizeSmall
            font.bold: !Common.isFileCanceled(event_type)
            horizontalAlignment: alignmentForEvent(event_type)
            color: colorForEventMsg(event_type)
        } // text
    } // row

    ProgressBar {
        anchors {
            left: parent.left
            right: parent.right
        }

        function labelText() {
            var sizeText = Common.humanFileSize(file_position, true) + "/" + Common.humanFileSize(file_size, true)
            if (file_size === 18446744073709551615) {
                sizeText = qsTr("Unkown", "file size")
            }

            return sizeText
        }

        indeterminate: file_size === 18446744073709551615 // "streaming" unknown size file

        minimumValue: 0
        maximumValue: file_size
        value: file_position
        label: labelText()
    }

    Timer {
        running: eventmodel.friendID >= 0 && Common.isFileRunning(event_type) // run only when downloading this file
        repeat: true
        interval: 1000 // 1x a second
        onTriggered: eventmodel.refreshFilePosition(index)
    }
} // main column
