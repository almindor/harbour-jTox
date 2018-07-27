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

Dialog {
    property string requestedName: "";
    property int lookupID: -1;

    enabled: toxcore.status > 0

    canAccept: (idField.text.length == 76 && !idField.errorHighlight && /^[0-9a-fA-F]{76}$/.test(idField.text))

    onAccepted: {
        friendInputField.focus = false
        messageField.focus = false
        friendmodel.addFriend(idField.text, messageField.text || messageField.placeholderText)
    }

    onVisibleChanged: {
        if ( visible ) {
            friendInputField.text = ''
            idField.text = ''
            messageField.text = ''
            friendInputField.focus = true
        } else {
            friendInputField.focus = false
        }
    }

    DialogHeader {
        id: dfdHeader
        title: qsTr("Add Friend")
    }

    TextField {
        id: friendInputField
        anchors {
            top: dfdHeader.bottom
            left: parent.left
            right: parent.right
        }

        label: qsTr("Press return to query") + " " + toxme.domain
        placeholderText: qsTr("Address or handle@") + toxme.domain
        errorHighlight: true

        onTextChanged: {
            var address_pat = /^[0-9a-fA-F]{76}$/;
            if ( text.length === 76 && address_pat.test(text) ) { // address is valid format
                idField.errorHighlight = false
                idField.text = text
                friendInputField.errorHighlight = false
                messageField.focus = true
            }
        }

        EnterKey.onClicked: {
            if ( text.length ) {
                var fqdn = text
                if ( text.indexOf('@') < 0 ) {
                    fqdn = text + '@' + toxme.domain
                }

                addFriendDialog.requestedName = fqdn
                addFriendDialog.lookupID = toxme.lookup(fqdn) // initiate lookup
            } else {
                idField.text = ""
                idField.errorHighlight = true
                friendInputField.errorHighlight = true
            }
        }
    }

    TextField {
        id: messageField
        anchors {
            top: friendInputField.bottom
            left: parent.left
            right: parent.right
        }

        label: qsTr("Message")
        placeholderText: qsTr("Please add me to friends")
    }

    TextArea {
        id: idField
        readOnly: true
        anchors {
            top: messageField.bottom
            left: parent.left
            right: parent.right
        }

        label: qsTr("Address")
        placeholderText: qsTr("Address not resolved")
        errorHighlight: true

        Connections {
            target: toxme
            onLookupDone: {
                if ( requestID != addFriendDialog.lookupID ) return; // someone else is using toxme
                idField.errorHighlight = false
                friendInputField.errorHighlight = false
                idField.text = toxID
                idField.label = qsTr("Resolved Address")
                if ( !messageField.text.length ) {
                    messageField.focus = true
                }
            }

            onRequestError: {
                if ( requestID != addFriendDialog.lookupID ) return; // someone else is using toxme

                friendInputField.errorHighlight = true
                idField.errorHighlight = true
                idField.text = error
                idField.label = qsTr("Resolve Error")
            }
        }
    }
}
