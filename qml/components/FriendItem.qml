import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: mainItem
    anchors {
        left: parent.left
        right: parent.right
    }

    contentHeight: Math.max(nameLabel.height + smText.height + Theme.paddingSmall*2, 64)

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

        MenuItem {
            text: qsTr("Details")
            onClicked: {
                friendmodel.setActiveFriendID(friend_id)
                pageStack.push(detailsDialog)
            }
        }
    }

    RemorseItem {
        id: removeRemorse
    }

    Label {
        id: nameLabel
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            topMargin: Theme.paddingSmall
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
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            bottomMargin: Theme.paddingSmall
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
            top: mainItem.top
            bottom: mainItem.bottom
            right: parent.right
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
    }
}
