import QtQuick 2.0
import Sailfish.Silica 1.0
import "../js/common.js" as Common

ListItem {
    id: listItem
    anchors {
        left: parent.left
        right: parent.right
        rightMargin: Theme.paddingLarge
        leftMargin: Theme.paddingLarge
    }

    contentHeight: createdLabel.height + lineContent.height

    onClicked: {
        if (Common.isCall(event_type)) {
            // do NOT start call here, just go to call page
            pageStack.navigateForward(PageStackAction.Animated)
        }
    }

    menu: ContextMenu {
        id: clipMenu

        MenuItem {
            text: qsTr("Call")
            visible: Common.isCall(event_type)
            onClicked: {
                // TODO: make the call
                pageStack.navigateForward(PageStackAction.Animated)
            }
        }

        MenuItem {
            text: qsTr("Copy to clipboard")
            visible: Common.isMessage(event_type)
            onClicked: {
                Clipboard.text = message
                banner("im", qsTr("Message copied to clipboard"), undefined, undefined, true)
            }
        }

        MenuItem {
            visible: event_type === Common.EventType.MessageOutOffline
            text: qsTr("Delete message")
            onClicked: eventmodel.deleteMessage(event_id)
        }

        MenuItem {
            visible: Common.isFilePending(event_type) && (file_pausers & 0x1) !== 0 // paused on our side
            text: (event_type === Common.EventType.FileTransferIn ? qsTr("Accept transfer") : qsTr("Resume transfer"))
            onClicked: {
                if ( Common.isFilePaused(event_type) || !eventmodel.fileExists(event_id) ) {
                    eventmodel.resumeFile(event_id)
                } else { // if file exists initially we must warn about override
                    overrideRemorse.execute(listItem, qsTr("Overriding") + " " + file_name, function() {
                        if ( eventmodel.deleteFile(event_id) ) {
                            eventmodel.resumeFile(event_id)
                        }
                    })
                }
            }
        }

        MenuItem {
            visible: Common.isFileRunning(event_type)
            text: qsTr("Pause transfer")
            onClicked: eventmodel.pauseFile(event_id)
        }

        MenuItem {
            visible: Common.isFileActive(event_type) || Common.isFilePaused(event_type)
            text: qsTr("Cancel transfer")
            onClicked: eventmodel.cancelFile(event_id)
        }

        MenuItem {
            visible: Common.isFile(event_type) && (Common.isFileDone(event_type) || Common.isEventOutgoing(event_type))
            text: qsTr("Open file")
            onClicked: Qt.openUrlExternally(file_path)
        }
    }

    RemorseItem {
        id: overrideRemorse
    }

    function colorForEventLabel(et) {
        if (Common.isEventIncoming(et)) {
            return Theme.secondaryColor
        }
        return Theme.secondaryHighlightColor
    }

    function colorForEventMsg(et, inverted) {
        var index = 0;
        var colors = [Theme.highlightColor, Theme.primaryColor];
        if (Common.isEventIncoming(et)) {
            index = 1;
        }

        if (inverted) {
            index = 1 - index;
        }

        return colors[index];
    }

    function colorForEventCall(et) {
        var index = 0;
        if (Common.isCallRejected(et)) {
            return 'red'
        }

        return colorForEventMsg(et)
    }

    function alignmentForEvent(et) {
        if (Common.isEventIncoming(et)) {
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

        text: Common.prettifyDateTime(created_at)
        font.pixelSize: Theme.fontSizeTiny
        horizontalAlignment: alignmentForEvent(event_type)
        color: colorForEventLabel(event_type)

        BusyIndicator {
            anchors {
                right: !Common.isEventIncoming(event_type) ? parent.right : undefined
                left: Common.isEventIncoming(event_type) ? parent.left : undefined
            }
            size: BusyIndicatorSize.ExtraSmall
            visible: Common.isEventPending(event_type)
            running: Common.isEventPending(event_type)
        }
    }

    Loader {
        id: lineContent
        source: Common.qmlForEventType(event_type)

        anchors {
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            top: createdLabel.bottom
        }
    }
}
