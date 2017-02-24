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
import org.nemomobile.notifications 1.0
import org.nemomobile.dbus 2.0
import "pages"

ApplicationWindow
{
    id: appWindow
    property int activeFriendID : -1 // used to reset model on messages
    initialPage: Component { Password { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: Orientation.Portrait
    _defaultPageOrientations: Orientation.Portrait
    onApplicationActiveChanged: toxcore.setApplicationActive(applicationActive);

    Timer {
        id: typingTimer
        property int friendID : -1
        interval: 2000
        onTriggered: eventmodel.stopTyping(friendID)
        running: false
        function watchTyping() {
            friendID = eventmodel.friendID
            start()
        }
    }

    Connections {
        target: toxcore
        onFriendRequest: banner("x-nemo.messaging.authorizationrequest", qsTr("New friend request"), "friend", appWindow.applicationActive)
        onAccountImported: banner("im", qsTr("Account Imported"), undefined, undefined, true)
        onAccountExported: banner("im", qsTr("Account Exported: ") + fileName, undefined, undefined, true)
        onLogsWiped: banner("im", qsTr("Chat history wiped"), undefined, undefined, true)
        onErrorOccurred: banner("x-nemo.messaging.error", error, "error", [error], appWindow.applicationActive)
    }

    Connections {
        target: friendmodel
        onFriendAddError: banner("x-nemo.messaging.error", error, "error", [error], appWindow.applicationActive)
    }

    Connections {
        target: eventmodel
        onMessageReceived: banner("x-nemo.messaging.im", qsTr("Message from") + " " + friendName, "message", [friendIndex], appWindow.applicationActive)
    }

    Connections {
        target: toxme
        onRequestError: banner("network.error", error, "error", [error], appWindow.applicationActive)
        onAlreadyRegistered: banner("network.error", "Already registered on toxme.io", undefined, undefined, appWindow.applicationActive)
        onPushDone: banner("network", qsTr("Registered on ") + toxme.domain, undefined, undefined, appWindow.applicationActive)
        onDeleteDone: banner("network", qsTr("Removed from ") + toxme.domain, undefined, undefined, appWindow.applicationActive)
    }

    DBusAdaptor {
        id: dbus
        service: 'net.almindor.jtox'
        iface: 'net.almindor.jtox'
        path: '/api'

        xml: '  <interface name="net.almindor.jtox">\n' +
             '    <method name="error" />\n' +
             '    <method name="friend" />\n' +
             '    <method name="message" />\n' +
             '  </interface>\n'

        function error(err) {
            console.error("Error: " + err) // TODO?
        }

        function friend() {
            if ( !appWindow.applicationActive ) {
                appWindow.activate()
            }

            // jump up to friends page
            while ( pageStack.depth > 1 ) {
                pageStack.pop(null, PageStackAction.Immediate)
            }
        }

        function message(friendIndex) {
            if ( !appWindow.applicationActive ) {
                appWindow.activate()
            }

            // jump up to friends page
            while ( pageStack.depth > 1 ) {
                pageStack.pop(null, PageStackAction.Immediate)
            }

            appWindow.activeFriendID = eventmodel.setFriendIndex(friendIndex)
            pageStack.push(Qt.resolvedUrl("pages/Messages.qml"), null, PageStackAction.Immediate)
        }
    }

    Notification {
        id: n
        appName: "jTox"
    }

    function banner(category, message, method, args, preview) {
        // skip message banners if set to busy
        if ( toxcore.status === 3 && category === "im.received" ) {
            return console.log("skipping notification: " + method + "(" + message + ")")
        }

        n.close()
        if ( preview ) {
            n.previewBody = message
            n.previewSummary = "jTox"
            n.body = ''
            n.summary = ''
        } else {
            n.body = message
            n.summary = "jTox"
            n.previewBody = message
            n.previewSummary = "jTox"
        }
        n.category = category

        if ( method ) {
            n.remoteActions = [{
                                   "name": "default",
                                   "displayName": "Do something",
                                   "icon": "icon-s-do-it",
                                   "service": "net.almindor.jtox",
                                   "path": "/api",
                                   "iface": "net.almindor.jtox",
                                   "method": method,
                                   "arguments": args
                               }];
        }

        n.publish()
    }
}

