import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/common.js" as Common

Text {
    linkColor: Theme.highlightColor
    textFormat: Text.AutoText
    onLinkActivated: Qt.openUrlExternally(link)

    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    font.pixelSize: Theme.fontSizeSmall

    text: message
    font.bold: event_type === Common.EventType.MessageInUnread
    horizontalAlignment: alignmentForEvent(event_type)
    color: colorForEventMsg(event_type)
}
