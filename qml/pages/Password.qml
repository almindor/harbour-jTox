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

Page {
    id: page
    onVisibleChanged: {
        if ( visible ) {
            passField.focus = true
        }
    }

    SilicaFlickable {
        anchors.fill: parent

        BusyIndicator {
            size: BusyIndicatorSize.Large
            x: page.width / 2.0 - width / 2.0
            y: page.height / 2.0 - height / 2.0
            z: 99
            visible: toxcore.busy
            running: toxcore.busy
        }

        RemorsePopup {
            id: remorseNA
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("New Account")
                onClicked: remorseNA.execute(qsTr("Creating Account"), function() {
                    toxcore.newAccount()
                } )
            }
        }

        TextField {
            id: passField
            enabled: !toxcore.busy
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }

            label: text.length < 8 ? qsTr("Password too short") : ""
            placeholderText: toxcore.initialUse ? qsTr("New account password") : qsTr("Account password")
            errorHighlight: false
            echoMode: TextInput.Password

            EnterKey.onClicked: {
                if ( text.length >= 8 ) {
                    toxcore.validatePassword(text)
                }
            }

            Connections {
                target: toxcore
                onPasswordValidChanged: {
                    if ( valid ) {
                        passField.errorHighlight = false
                        passField.focus = false
                        toxcore.init(passField.text)
                        pageStack.replaceAbove(null, Qt.resolvedUrl("Friends.qml"))
                    } else {
                        passField.errorHighlight = true
                        passField.label = qsTr("Invalid password")
                    }
                }
            }
        }
    }
}
