import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/common.js" as Common

Row {
    spacing: Theme.paddingSmall
    layoutDirection: Common.isMessageIncoming(event_type) ? Qt.RightToLeft : Qt.LeftToRight
    enabled: !Common.isFilePaused(event_type)

    Text {
        text: "📎"
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        font.pixelSize: Theme.fontSizeMedium
        font.bold: false
        font.strikeout: Common.isFileCanceled(event_type)
        horizontalAlignment: alignmentForEvent(event_type)
        color: colorForEventMsg(event_type)

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: Common.isFileRunning(event_type)

            NumberAnimation {
                from: 1.0
                to: 0.5
                duration: 1000
            }
            NumberAnimation {
                from: 0.5
                to: 1.0
                duration: 1000
            }
        }
    }

    Column {
        Text {
            text: message
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.strikeout: Common.isFileCanceled(event_type)
            font.pixelSize: Theme.fontSizeSmall
            font.bold: Common.isFileActive(event_type)
            horizontalAlignment: alignmentForEvent(event_type)
            color: colorForEventMsg(event_type)

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: Common.isFilePending(event_type)

                NumberAnimation {
                    from: 1.0
                    to: 0.5
                    duration: 1000
                }
                NumberAnimation {
                    from: 0.5
                    to: 1.0
                    duration: 1000
                }
            }
        }

        Text {
            anchors { // let this spread to name size
                left: parent.left
                right: parent.right
            }

            text: Common.humanFileSize(file_size, true)
            font.strikeout: Common.isFileCanceled(event_type)
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.pixelSize: Theme.fontSizeTiny
            font.bold: Common.isFileActive(event_type)
            horizontalAlignment: alignmentForEvent(event_type)
            color: Theme.highlightBackgroundColor
        }
    }
}

