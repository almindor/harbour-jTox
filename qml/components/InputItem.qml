import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    anchors {
        left: parent.left
        right: parent.right
    }
    height: inputField.height + typingItem.height
    signal sendMessage(string msg)

    Item {
        id: typingItem
        visible: eventmodel.friendTyping
        height: uti.height
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }

        Text {
            text: eventmodel.friendName.substring(0, 64) + " " + qsTr("is typing")
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
            anchors {
                left: parent.left
                right: uti.left
            }
        }

        UserTypingIndicator {
            id: uti
            anchors {
                right: parent.right
            }
        }
    }

    TextField {
        id: inputField
        enabled: toxcore.status > 0 && eventmodel.friendStatus > 0
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        placeholderText: qsTr("Type your message here")
        EnterKey.onClicked: {
            sendMessage(text)
            text = ''
            eventmodel.typing = false
            if ( typingTimer.running ) {
                typingTimer.stop()
            }
        }

        onTextChanged: {
            if ( typingTimer.running ) {
                typingTimer.stop()
            }

            if ( text.length === 0 ) {
                eventmodel.typing = false
                return
            }

            eventmodel.typing = true
            typingTimer.watchTyping()
        }
    }
}
