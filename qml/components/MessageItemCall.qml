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
            id: callIcon
            source: "image://theme/icon-m-answer?" + colorForEventCall(event_type)
        }

        Text {
            width: mainColumn.width - callIcon.width - Theme.paddingSmall
            text: Common.msgForCall(event_type)
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.strikeout: Common.isCallRejected(event_type)
            font.pixelSize: Theme.fontSizeSmall
            font.bold: true
            horizontalAlignment: alignmentForEvent(event_type)
            color: colorForEventMsg(event_type)
        } // text
    } // row
} // main column
