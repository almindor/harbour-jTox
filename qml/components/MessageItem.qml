import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/common.js" as Common

ListItem {
    id: listItem
    anchors {
        left: parent.left
        right: parent.right
    }

    contentHeight: createdLabel.height + lineContent.height

    menu: ContextMenu {
        id: clipMenu

        MenuItem {
            text: qsTr("Copy to clipboard")
            visible: Common.isMessage(event_type)
            onClicked: {
                Clipboard.text = messageText.text
                banner("im", qsTr("Message copied to clipboard"), undefined, undefined, true)
            }
        }

        MenuItem {
            visible: event_type === Common.EventType.MessageOutOffline
            text: qsTr("Delete message")
            onClicked: eventmodel.deleteMessage(event_id)
        }

        MenuItem {
            visible: Common.isFilePending(event_type) && Common.isMessageIncoming(event_type)
            text: qsTr("Accept transfer")
            onClicked: eventmodel.acceptFile(event_id)
        }

        MenuItem {
            visible: Common.isFilePending(event_type)
            text: Common.isMessageIncoming(event_type) ? qsTr("Reject transfer") : qsTr("Cancel transfer")
            onClicked: eventmodel.cancelFile(event_id)
        }
    }

    function colorForEventLabel(et) {
        if (Common.isMessageIncoming(et)) {
            return Theme.secondaryColor
        }
        return Theme.secondaryHighlightColor
    }

    function colorForEventMsg(et, inverted) {
        var index = 0;
        var colors = [Theme.highlightColor, Theme.primaryColor];
        if (Common.isMessageIncoming(et)) {
            index = 1;
        }

        if (inverted) {
            index = 1 - index;
        }

        return colors[index];
    }

    function alignmentForEvent(et) {
        if (Common.isMessageIncoming(et)) {
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
/*
    Text {
        id: lineContent
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }
        anchors.top: createdLabel.bottom

        linkColor: Theme.highlightColor
        textFormat: Text.RichText
        onLinkActivated: Qt.openUrlExternally(link)

        text: message
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        font.pixelSize: Theme.fontSizeSmall
        font.bold: event_type == 2
        horizontalAlignment: alignmentForEvent(event_type)
        color: colorForEventMsg(event_type)
    }*/

    Loader {
        id: lineContent
        source: switch(Common.isFile(event_type)) {
            case true: return "MessageItemFile.qml"
            case false: return "MessageItemText.qml"
        }

        anchors {
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            top: createdLabel.bottom
        }
    }
}
