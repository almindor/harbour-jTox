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
import org.nemomobile.configuration 1.0

Page {
    id: page
    allowedOrientations: Orientation.Portrait

    ConfigurationValue {
        id: multilineMessages
        defaultValue: true
        key: "/multilineMessages"
    }

    RemorsePopup {
        id: remorseIE
    }

    RemorsePopup {
        id: remorseCH
    }

    Import {
        id: importDialog

        onAccepted: remorseIE.execute(qsTr("Importing Account"), function() {
            // if we import ok, kick off to login
            if ( toxcore.importAccount(fileName) ) {
                pageStack.clear()
                pageStack.push(Qt.resolvedUrl("Password.qml"), null, PageStackAction.Immediate)
            }
        } )
    }

    SilicaFlickable {
        id: main
        anchors.fill: parent
        contentHeight: col.height

        VerticalScrollDecorator {
            flickable: main
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Import Account")
                onClicked: pageStack.push(importDialog)
            }

            MenuItem {
                text: qsTr("Export Account")
                onClicked: remorseIE.execute(qsTr("Exporting Account"), function() {
                    toxcore.exportAccount()
                } )

            }
        }

        Column {
            id: col

            width: page.width
            spacing: Theme.paddingLarge
            anchors {
                left: parent.left
                right: parent.right
            }

            PageHeader {
                title: qsTr("Account")
            }

            Button {
                text: qsTr("Show QR Code")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: pageStack.push(Qt.resolvedUrl("QRID.qml"))
            }

            BackgroundItem {
                anchors {
                    left: parent.left
                    right: parent.right
                }

                onClicked: {
                    Clipboard.text = toxcore.toxID
                    banner("im", qsTr("ToxID copied to clipboard"), undefined, undefined, true)
                }

                Column {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }

                    Label {
                        id: toxidLabel
                        anchors {
                            left: parent.left
                            right: parent.right
                            margins: Theme.paddingLarge
                        }
                        wrapMode: TextInput.WrapAnywhere
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeTiny
                        text: toxcore.toxID
                    }

                    Text {
                        anchors {
                            left: parent.left
                            right: parent.right
                            margins: Theme.paddingLarge
                        }

                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        text: qsTr("Tox ID: touch to copy to clipboard")
                    }
                }
            }

            TextField {
                id: noSpamField
                anchors {
                    left: parent.left
                    right: parent.right
                }
                text: toxcore.noSpam
                label: qsTr("Nospam")
                labelVisible: true
                placeholderText: qsTr("Nospam")
                EnterKey.onClicked: toxcore.setNoSpam(noSpamField.text)
            }

            TextField {
                id: nameField
                anchors {
                    left: parent.left
                    right: parent.right
                }
                text: toxcore.userName
                label: qsTr("User name")
                labelVisible: true
                placeholderText: qsTr("User name")
                EnterKey.onClicked: toxcore.userName = nameField.text
            }

            TextField {
                id: statusField
                anchors {
                    left: parent.left
                    right: parent.right
                }
                text: toxcore.statusMessage
                label: qsTr("Status message")
                placeholderText: qsTr("Hi from jTox on Sailfish")
                EnterKey.onClicked: toxcore.statusMessage = statusField.text
            }

            SectionHeader {
                text: qsTr("Chat")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Wipe history")
                onClicked: remorseCH.execute(qsTr("Wiping chat history"), function() {
                    toxcore.wipeLogs()
                } )
            }

            TextSwitch {
               id: multilineMessagesSwitch
               checked: multilineMessages.value
               width: parent.width
               text: qsTr("Multiline messages")
               onClicked: multilineMessages.value = !multilineMessages.value
            }

            SectionHeader {
                text: toxme.domain
            }

            TextField {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                }

                visible: toxme.password.length > 0
                text: toxme.password
                label: toxme.domain + " " + qsTr("password")
                readOnly: true
            }

            TextSwitch {
                id: privToggle
                text: qsTr("Private")
                checked: toxme.isPrivate
            }

            Item {
                height: toxmeButton.height + Theme.paddingLarge
                width: toxmeButton.width + untoxmeButton.width + Theme.paddingMedium
                anchors {
                    horizontalCenter: parent.horizontalCenter
                }

                Button {
                    id: toxmeButton
                    anchors {
                        left: parent.left
                    }

                    text: toxme.password.length ? qsTr("Update") : qsTr("Register")
                    onClicked: {
                        toxme.push(nameField.text, toxcore.toxID, 'TODO', privToggle.checked)
                    }
                }

                Button {
                    id: untoxmeButton
                    anchors {
                        right: parent.right
                    }

                    text: qsTr("Delete")
                    onClicked: {
                        toxme.remove()
                    }
                }
            }
        }
    }
}
