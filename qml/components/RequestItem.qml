import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: bgItem
    anchors {
        left: parent.left
        right: parent.right
        margins: Theme.paddingLarge
    }

    height: Math.max(nameLabel.height + smText.height, 64) + acceptButton.height

    Button {
        id: acceptButton
        anchors {
            left: parent.left
            bottom: parent.bottom
        }

        text: qsTr("Accept")
        onClicked: remorseItem.execute(parent, qsTr('Accepting') + ' ' + nameLabel.text, function() {
            requestmodel.accept(index)
            pageStack.pop()
        })

    }

    Button {
        id: rejectButton
        anchors {
            right: parent.right
            bottom: parent.bottom
        }

        text: qsTr("Reject")
        onClicked: remorseItem.execute(parent, qsTr('Rejecting') + ' ' + nameLabel.text, function() {
            requestmodel.reject(index)
            pageStack.pop()
        })
    }

    Label {
        id: nameLabel
        anchors {
            left: parent.left
            top: parent.top
        }

        text: name || address
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width: page.width - 64 - Theme.paddingSmall
        font.pixelSize: Theme.fontSizeSmall
    }

    TextArea {
        id: smText
        readOnly: true
        color: Theme.highlightColor
        anchors {
            left: parent.left
            top: nameLabel.bottom
        }
        text: message
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        width: page.width - 64 - Theme.paddingSmall
        font.pixelSize: Theme.fontSizeTiny
    }

    RemorseItem {
        id: remorseItem
    }
}
