import QtQuick 2.0
import Sailfish.Silica 1.0

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
                console.log("LIH: " + listItem.height)
                console.log("col: " + col.height)
                console.log("MT: " + messageText.height)
                clipboard.setText(messageText.text)
            }
        }
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

        // this fugly hack is needed otherwise the listitem height is set wrong
        /*Component.onCompleted: {
            listItem.height = col.height
        }*/

        text: message
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        font.pixelSize: Theme.fontSizeSmall
        font.bold: event_type == 2
        horizontalAlignment: (event_type == 2 || event_type == 4) ? Text.AlignRight : Text.AlignLeft
        color: (event_type == 2 || event_type == 4) ? Theme.primaryColor : Theme.highlightColor
    }
}
