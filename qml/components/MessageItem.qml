import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/common.js" as Common

ListItem {
    id: listItem
    anchors {
        left: parent.left
        right: parent.right
    }

    contentHeight: createdLabel.height + messageText.height

    menu: ContextMenu {
        id: clipMenu

        MenuItem {
            text: qsTr("Copy to clipboard")
            onClicked: {
                Clipboard.text = messageText.text
                banner("im", qsTr("Message copied to clipboard"), undefined, undefined, true)
            }
        }

        MenuItem {
            visible: event_type === Common.EventType.MessageOutOffline
            text: qsTr("Delete message")
            onClicked: {
                eventmodel.deleteMessage(event_id)
            }
        }
    }

    function colorForEventLabel(et) {
        if (et === Common.EventType.MessageInUnread || et === Common.EventType.MessageIn) {
            return Theme.secondaryColor
        }
        return Theme.secondaryHighlightColor
    }

    function colorForEventMsg(et) {
        if (et === Common.EventType.MessageInUnread || et === Common.EventType.MessageIn) {
            return Theme.primaryColor
        }
        return Theme.highlightColor
    }

    function alignmentForEvent(et) {
        if (et === Common.EventType.MessageInUnread || et === Common.EventType.MessageIn) {
            return Text.AlignRight
        }

        return Text.AlignLeft
    }

    Text {
        id: createdLabel
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
        }

        text: created_at
        font.pixelSize: Theme.fontSizeTiny
        horizontalAlignment: alignmentForEvent(event_type)
        color: colorForEventLabel(event_type)

        BusyIndicator {
            anchors {
                right: parent.right
            }
            size: BusyIndicatorSize.ExtraSmall
            visible: Common.isMessagePending(event_type)
            running: Common.isMessagePending(event_type)
        }
    }

    Text {
        id: messageText
        anchors {
            left: parent.left
            right: parent.right
            top: createdLabel.bottom
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }

        linkColor: Theme.highlightColor
        textFormat: Text.RichText
        onLinkActivated: {
            Qt.openUrlExternally(link)
        }

        text: message
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        font.pixelSize: Theme.fontSizeSmall
        font.bold: event_type == 2
        horizontalAlignment: alignmentForEvent(event_type)
        color: colorForEventMsg(event_type)
    }
}
