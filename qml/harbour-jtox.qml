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
import QtFeedback 5.0
import QtMultimedia 5.6
import Nemo.DBus 2.0
import Nemo.Notifications 1.0
import "pages"
import "js/common.js" as Common

ApplicationWindow
{
    id: appWindow
    property int activeFriendID : -1 // used to reset model on messages
    initialPage: Component { Password { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: Orientation.Portrait
    _defaultPageOrientations: Orientation.Portrait
    onApplicationActiveChanged: toxcore.setApplicationActive(applicationActive);

    // plays when someone is calling us and we didn't pick up yet
    MediaPlayer {
        id: incomingTone
        source: "file:///usr/share/sounds/jolla-ringtones/stereo/jolla-ringtone.ogg" // TODO: make configurable
        loops: MediaPlayer.Infinite
        // audioRole: MediaPlayer.RingtoneRole // does nothing atm. but one can hope
    }

    // plays when we're calling someone and they didn't pick up yet (beep... beep...)
    MediaPlayer {
        id: outgoingTone
        source: "sounds/calling.ogg"
        loops: MediaPlayer.Infinite
        // audioRole: MediaPlayer.RingtoneRole // does nothing atm. but one can hope
    }

    // plays once when we get a reject from other side
    MediaPlayer {
        id: busyTone
        source: "sounds/busy.ogg"
        loops: 1
        // audioRole: MediaPlayer.RingtoneRole // does nothing atm. but one can hope
    }

    HapticsEffect {
        id: vibrate
        duration: 750
        intensity: 1.0
    }

    Timer {
        onTriggered: vibrate.start()
        interval: 1500
        running: toxcoreav.callIsIncoming && toxcoreav.globalCallState === 1 // incoming and ringing
        onRunningChanged: {
            if (running) {
                vibrate.start()
            }
        }

        repeat: true
    }

    DBusInterface {
        id: mce

        bus: DBus.SystemBus
        service: 'com.nokia.mce'
        iface: 'com.nokia.mce.request'
        path: '/com/nokia/mce/request'

        function setCallState(state) {
            var strState = 'none'
            switch (state) {
                case 0: strState = 'none'; break;
                case 1: strState = 'ringing'; break;
                case 2: strState = 'active'; break;
                default: console.error('Unknown MCE state from ToxCoreAV'); break;
            }

            // Telephony.audioMode = 'earpiece'
            mce.call('req_call_state_change', [strState, 'normal'],
                     function(result) {
                         console.log('mce call state set:', strState, 'result:', result)
                     },
                     function(error, message) {
                         console.error('mce call state set to:', strState, 'failed error:', error, 'message:', message)
                     })
        }
    }

    Connections {
        target: toxcore
        onFriendRequest: banner("x-nemo.messaging.authorizationrequest", qsTr("New friend request"), "friend", appWindow.applicationActive)
        onAccountImported: banner("im", qsTr("Account Imported"), undefined, undefined, true)
        onAccountExported: banner("im", qsTr("Account Exported: ") + fileName, undefined, undefined, true)
        onAccountCreated: banner('im', qsTr("New account created"), undefined, undefined, true)
        onLogsWiped: banner("im", qsTr("Chat history wiped"), undefined, undefined, true)
        onErrorOccurred: banner("x-nemo.messaging.error", error, "error", [error], appWindow.applicationActive)
    }

    Connections {
        id: avConns
        target: toxcoreav
        onCalledBusy: busyTone.play()
        onGlobalCallStateChanged: {
            // make sure to use earpiece when in call or when we're calling and it's ringing on their end
            var port = state === 2 ? Common.AudioPorts.Earpiece : Common.AudioPorts.Speaker // TODO: make sure to not lock out bluetooth, jack and usb outputs
            if (state === 1 && !toxcoreav.callIsIncoming) { // outgoing ringing on their side
                port = Common.AudioPorts.Earpiece;
            }

            sinkPortModel.selectPort(port) // TODO: fix when Jolla finally tells us how to get voicecall sink setup right
            mce.setCallState(state) // TODO: currently succeeds but doesn't work with this sink/setup on the MCE side (ignored proximity)

            if (state !== 1) {
                console.error('Stopping ringtones')
                incomingTone.stop()
                outgoingTone.stop()
                incomingTone.seek(0)
                outgoingTone.seek(0)
                return
            }

            if (toxcoreav.callIsIncoming) {
                incomingTone.seek(0)
                incomingTone.play()
            } else {
                outgoingTone.seek(0)
                outgoingTone.play()
            }
        }
        onErrorOccurred: banner("x-nemo.messaging.error", error, "error", [error], appWindow.applicationActive)
    }

    Connections {
        target: friendmodel
        onFriendAddError: banner("x-nemo.messaging.error", error, "error", [error], appWindow.applicationActive)
    }

    Connections {
        target: eventmodel
        onIncomingCall: Common.jumpToCall(appWindow, pageStack, eventmodel, null, friendIndex, PageStackAction.Immediate)
        onMessageReceived: banner("x-nemo.messaging.im", qsTr("Message from") + " " + friendName, "message", [friendIndex], appWindow.applicationActive)
        onTransferReceived: banner("x-nemo.messaging.mms", qsTr("Transfer from") + " " + friendName, "message", [friendIndex], appWindow.applicationActive)
        onTransferComplete: banner("x-nemo.transfer.complete", qsTr("File transfer complete"), "message", [friendIndex], appWindow.applicationActive)
        onTransferError: banner("x-nemo.transfer.error", error, "error", [error], appWindow.applicationActive)
        onEventError: banner("x-nemo.messaging.error", error, "error", [error], appWindow.applicationActive)
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

