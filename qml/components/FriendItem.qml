import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: mainItem
    anchors {
        left: parent.left
        right: parent.right
        margins: Theme.paddingLarge
    }
    contentHeight: Math.max(nameLabel.height + smText.height, 64)

    onClicked: {
        appWindow.activeFriendID = friend_id
        eventmodel.setFriend(friend_id)
        pageStack.push(Qt.resolvedUrl("../pages/Messages.qml"), { friend_id: friend_id })
    }

    menu: ContextMenu {
        id: removeMenu

        MenuItem {
            text: qsTr("Remove")
            onClicked: removeRemorse.execute(mainItem, qsTr("Removing") + " " + name, function() {
                friendmodel.removeFriend(friend_id)
            })
        }
    }

    RemorseItem {
        id: removeRemorse
    }

    Label {
        id: nameLabel
        anchors {
            left: parent.left
            top: parent.top
        }

        text: name
        font.bold: unviewed
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width: page.width - 64 - Theme.paddingSmall
        font.pixelSize: Theme.fontSizeSmall
    }

    Text {
        id: smText
        color: Theme.highlightColor
        anchors {
            left: parent.left
            bottom: parent.bottom
        }
        text: status_message
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width: page.width - 64 - Theme.paddingSmall
        font.pixelSize: Theme.fontSizeTiny
    }

    UserTypingIndicator {
        anchors {
            right: parent.right
            bottom: parent.bottom
        }

        visible: typing
    }

    UserStatusIndicator {
        userStatus: status
        anchors {
            top: parent.top
            right: parent.right
        }
    }
}
