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

    TextArea {
        id: inputField
        enabled: toxcore.status > 0
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: sendButton.left
        }

        function shortened(name) {
            if ( name.length > 15 ) {
                return name.substring(0, 15) + '...';
            }

            return name;
        }

        placeholderText: toxcore.status > 0 && eventmodel.friendStatus > 0 ? qsTr("Type your message here") : (shortened(eventmodel.friendName) + " " + qsTr("is offline"))

        onTextChanged: {
            if ( text.length === 0 ) {
                eventmodel.typing = false
                return
            }

            eventmodel.typing = true
        }
    }

    IconButton {
        id: sendButton
        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: {
                bottom: Theme.paddingSmall
            }
        }

        icon.source: "image://theme/icon-m-forward"
        onClicked: {
            var text = inputField.text

            if ( text.length > 0 ) { // only send non-empties
                sendMessage(text)
            }

            inputField.text = ''
            eventmodel.typing = false
        }
    }

}
