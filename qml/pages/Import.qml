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
    id: dialog

    property string fileName
    canAccept: !toxcore.busy && fileName.length > 0

    DialogHeader {
        id: impHeader
        title: qsTr("Import Account")
    }

    TextArea {
        id: warningText
        anchors {
            left: parent.left
            right: parent.right
            top: impHeader.bottom
            topMargin: Theme.paddingMedium
        }

        readOnly: true
        errorHighlight: true
        placeholderText: qsTr("WARNING!")
        text: qsTr("Importing an account will delete existing account including contacts and messages!")
    }

    SilicaListView {
        id: listView
        property int selectedIndex : -1

        header: SectionHeader {
            id: pageHeader
            text: qsTr("Documents Folder")
        }

        footer: TextArea {
            visible: dirmodel.count === 0
            Component.onCompleted: { // this thing is dynamic and seems to have issues with direct assignments to anchors
                anchors.left = parent.left
                anchors.right = parent.right
            }

            readOnly: true
            placeholderText: qsTr("No account files found")
            text: qsTr("No account *.tox files found in Documents folder. Copy account file to Documents and try again.")
        }

        anchors {
            top: warningText.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        spacing: Theme.paddingSmall
        model: dirmodel
        VerticalScrollDecorator {
            flickable: listView
        }

        delegate: BackgroundItem {
            anchors {
                left: parent.left
                right: parent.right
            }

            TextSwitch {
                id: fileSwitch
                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    rightMargin: Theme.paddingLarge
                    leftMargin: Theme.paddingLarge
                }

                text: fileName
                checked: listView.selectedIndex === index

                onClicked: {
                    listView.selectedIndex = checked ? index : -1
                    dialog.fileName = checked ? text : ''
                }

                Connections {
                    target: listView
                    onSelectedIndexChanged: {
                        if ( listView.selectedIndex !== index && fileSwitch.checked ) {
                            fileSwitch.checked = false
                        }
                    }
                }
            }
        }
    }


}
