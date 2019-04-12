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
import Sailfish.Pickers 1.0
import "../components"
import org.nemomobile.configuration 1.0

Page {
    id: page
    allowedOrientations: Orientation.Portrait
    property bool checkFriendID: true

    onVisibleChanged: {
        if ( appWindow.activeFriendID < 0 ) {
            return; // nothing
        }

        if ( !checkFriendID && !visible ) {
            checkFriendID = true;
            return;
        }

        eventmodel.setFriend(visible ? appWindow.activeFriendID : -1)
        if ( visible ) {
            listView.positionViewAtBeginning()
        }
    }


    ConfigurationValue {
        id: multilineMessages
        defaultValue: true
        key: "/multilineMessages"
    }

    Component {
        id: filePickerPage
        ContentPickerPage {
            onSelectedContentPropertiesChanged: {
                eventmodel.sendFile(selectedContentProperties.filePath)
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: parent.height - Theme.paddingLarge
        clip: true

        PageHeader {
            id: pageHeader
            title: eventmodel.friendName

            UserStatusIndicator {
                id: usi
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }

                userStatus: eventmodel.friendStatus
            }

            Avatar {
                source_id: appWindow.activeFriendID
                width: parent.width / 8
                height: width
                placeholder: ""

                anchors {
                    verticalCenter: parent.verticalCenter
                    left: usi.right
                    leftMargin: Theme.paddingLarge
                }
            }
        }

        SilicaListView {
            id: listView
            spacing: Theme.paddingSmall
            anchors.fill: parent
            anchors.topMargin: pageHeader.height
            anchors.bottomMargin: multilineMessages.value ? multiTextField.height : altTextField.height

            verticalLayoutDirection: ListView.BottomToTop
            clip: true

            model: eventmodel
            VerticalScrollDecorator {
                flickable: listView
            }

            delegate: MessageItem {}
        }

        InputItem {
            id: multiTextField
            visible: multilineMessages.value
            anchors.bottom: parent.bottom
            onSendMessage: {
                eventmodel.sendMessage(msg)
            }
        }

        AltInputItem {
            id: altTextField
            visible: !multilineMessages.value
            anchors.bottom: parent.bottom
            onSendMessage: {
                eventmodel.sendMessage(msg)
            }
        }

        PushUpMenu {
            visible: toxcore.status > 0 && eventmodel.friendStatus > 0
            MenuItem {
                text: qsTr("Send file")
                onClicked: {
                    checkFriendID = false // ensure we don't lose friendID
                    pageStack.push(filePickerPage)
                }
            }
        }
    }
}


