import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    anchors {
        left: parent.left
        right: parent.right
    }

    Text {
        id: createdLabel
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
        }

        text: created_at
        font.pixelSize: Theme.fontSizeTiny
        horizontalAlignment: (event_type == 2 || event_type == 4) ? Text.AlignRight : Text.AlignLeft
        color: (event_type == 2 || event_type == 4) ? Theme.secondaryColor : Theme.secondaryHighlightColor

        BusyIndicator {
            anchors {
                right: parent.right
            }
            size: BusyIndicatorSize.ExtraSmall
            visible: event_type == 3
            running: event_type == 3
        }
    }

    TextArea {
        id: messageText
        anchors {
            left: parent.left
            right: parent.right
        }
        text: message
        readOnly: true
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        font.pixelSize: Theme.fontSizeSmall
        font.bold: event_type == 2
        horizontalAlignment: (event_type == 2 || event_type == 4) ? Text.AlignRight : Text.AlignLeft
        color: (event_type == 2 || event_type == 4) ? Theme.primaryColor : Theme.highlightColor
    }

}
