/*
    Copyright (C) 2016 Ales Katona.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Dialog {
    id: detailsDialog
    allowedOrientations: Orientation.Portrait

    DialogHeader {
        id: ddHeader
        title: qsTr("Details")
    }

    onAccepted: {
        friendmodel.setOfflineName(nameField.text)
    }

    Column {
        id: col

        width: detailsDialog.width
        spacing: Theme.paddingLarge
        anchors {
            top: ddHeader.bottom
            left: parent.left
            right: parent.right
        }

        Label {
            id: address
            anchors {
                left: parent.left
                right: parent.right
                margins: Theme.paddingLarge
            }
            wrapMode: TextInput.WrapAnywhere
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeTiny
            text: friendmodel.address
        }

        TextField {
            id: nameField
            anchors {
                left: parent.left
                right: parent.right
            }
            label: qsTr("Name")
            text: friendmodel.name
            labelVisible: true
            placeholderText: qsTr("Friend offline name")
        }
    }
}


