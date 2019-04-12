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

Page {
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: parent.height - Theme.paddingLarge

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
        }

        PullDownMenu {
            visible: toxcoreav.globalCallState === 1 // ringing
            MenuItem {
                text: qsTr("Answer")
                onClicked: toxcoreav.answerIncomingCall(appWindow.activeFriendID)
            }
        }

        PushUpMenu {
            visible: toxcoreav.globalCallState === 1 // ringing

            MenuItem {
                text: qsTr("Reject")
                onClicked: toxcoreav.endCall(appWindow.activeFriendID)
            }
        }

        Avatar {
            source_id: appWindow.activeFriendID
            width: parent.width / 2
            height: width
            placeholder: ""

            anchors {
                centerIn: parent
            }
        }

        Button {
            visible: toxcoreav.globalCallState === 0 // none
            text: qsTr("Call")
            onClicked: toxcoreav.callFriend(appWindow.activeFriendID)
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }
        }

        Button {
            visible: toxcoreav.globalCallState > 0 // ringing or calling
            text: qsTr("End Call")
            onClicked: toxcoreav.endCall(appWindow.activeFriendID)
            anchors {
                horizontalCenter: parent.horizontalCenter
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }
        }
    }
}
