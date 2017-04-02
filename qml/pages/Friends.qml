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
    id: page
    allowedOrientations: Orientation.Portrait

    BusyIndicator {
        size: BusyIndicatorSize.Large
        x: page.width / 2.0 - width / 2.0
        y: page.height / 2.0 - height / 2.0
        z: 99
        visible: toxcore.busy && appWindow.applicationActive
        running: toxcore.busy && appWindow.applicationActive
    }

    AddFriend {
        id: addFriendDialog
    }

    Details {
        id: detailsDialog
    }

    SilicaListView {
        id: listView
        header: PageHeader {
            title: qsTr("Friends")

            UserStatusIndicator {
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }

                userStatus: toxcore.status
                offlineBusy: toxcore.initialized
            }
        }

        footer: BackgroundItem {
            visible: requestmodel.size > 0
            enabled: toxcore.status > 0
            onClicked: pageStack.push(Qt.resolvedUrl("Requests.qml"))
            Component.onCompleted: { // this thing is dynamic and seems to have issues with direct assignments to anchors
                anchors.left = parent.left
                anchors.right = parent.right
            }

            Text {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Pending friend requests: ") + requestmodel.size
                color: toxcore.status > 0 ? Theme.highlightColor : Theme.highlightDimmerColor
                font.pixelSize: Theme.fontSizeLarge
            }
        }


        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("About.qml"))
            }

            MenuItem {
                text: qsTr("Account")
                onClicked: pageStack.push(Qt.resolvedUrl("Account.qml"))
            }

            MenuItem {
                text: qsTr("Add Friend")
                onClicked: pageStack.push(addFriendDialog)
                enabled: toxcore.status > 0
            }
        }

        PushUpMenu {
            MenuItem {
                text: qsTr("Ready")
                onClicked: toxcore.status = 1
                UserStatusIndicator {
                    userStatus: 1
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                    }
                }
            }

            MenuItem {
                text: qsTr("Away")
                onClicked: toxcore.status = 2
                UserStatusIndicator {
                    userStatus: 2
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                    }
                }
            }

            MenuItem {
                text: qsTr("Busy")
                onClicked: toxcore.status = 3
                UserStatusIndicator {
                    userStatus: 3
                    anchors {
                        left: parent.left
                        verticalCenter: parent.verticalCenter
                    }
                }
            }
        }

        anchors.fill: parent
        spacing: Theme.paddingLarge
        model: friendmodel
        VerticalScrollDecorator {
            flickable: listView
        }

        delegate: FriendItem {}
    }
}
