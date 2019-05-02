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
 import QtSensors 5.2
import "../components"

Page {
    id: callsPage
    allowedOrientations: Orientation.Portrait

    // TODO: remove when Jolla tells us how to set sinks right for MCE to do this
    Rectangle {
        id: blank

        visible: toxcoreav.globalCallState > 0 && proximitySensor.reading.near
        anchors.fill: parent
        color: "black"
    }

    SilicaFlickable {
        visible: !(toxcoreav.globalCallState > 0 && proximitySensor.reading.near) // TODO: remove when Jolla tells us how to set sinks right for MCE to do this
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
            visible: toxcoreav.globalCallState < 2

            MenuItem {
                visible: toxcoreav.globalCallState === 0 // none
                text: qsTr("Call")
                onClicked: toxcoreav.callFriend(appWindow.activeFriendID)
            }

            MenuItem {
                visible: toxcoreav.globalCallState === 1 && toxcoreav.callIsIncoming // ringing
                text: qsTr("Answer")
                onClicked: toxcoreav.answerIncomingCall(appWindow.activeFriendID)
            }
        }

        PushUpMenu {
            backgroundColor: "red"
            highlightColor: "red"

            visible: toxcoreav.globalCallState > 0 // ringing or in call

            MenuItem {
                color: "red"
                text: toxcoreav.globalCallState === 1 && toxcoreav.callIsIncoming ? qsTr("Reject") : qsTr("End Call")
                onClicked: toxcoreav.endCall(appWindow.activeFriendID)
            }
        }

        Column {
            spacing: Theme.paddingLarge
            y: parent.height / 3.0
            anchors.left: parent.left
            anchors.right: parent.right

            Text {
                function statusText(status, incoming) {
                    switch (status) {
                    case 1: return incoming ? qsTr("Incoming Call", "call state") : qsTr("Outgoing Call", "call state")
                    case 2: return qsTr("Ongoing Call", "call state")
                    }

                    return "";
                }

                anchors.horizontalCenter: parent.horizontalCenter
                color: Theme.highlightColor
                text: statusText(toxcoreav.globalCallState, toxcoreav.callIsIncoming)
                font.pixelSize: Theme.fontSizeExtraLarge
            }

            Avatar {
                anchors.horizontalCenter: parent.horizontalCenter
                source_id: appWindow.activeFriendID
                width: parent.width / 2
                height: width
                placeholder: ""
            }
        }

        // TODO: remove when Jolla tells us how to set sinks right for MCE to do this
        ProximitySensor {
            id: proximitySensor
            active: toxcoreav.globalCallState > 0 // ringing or in call
        }
    }
}
