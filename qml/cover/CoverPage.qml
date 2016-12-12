/*
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

CoverBackground {
    Item {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: Theme.paddingLarge
        }

        height: Math.max(usi.height, usit.height)

        UserStatusIndicator {
            id: usi
            anchors {
                left: parent.left
                verticalCenter: parent.verticalCenter
                leftMargin: Theme.paddingLarge
            }

            userStatus: toxcore.status
            offlineBusy: false
        }

        Text {
            id: usit
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                rightMargin: Theme.paddingLarge
            }

            color: toxcore.status > 0 ? Theme.primaryColor : Theme.secondaryColor
            text: toxcore.statusText
        }
    }

    Image {
        id: bgimg
        source: "../images/cover.png"
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: sourceSize.height * width / sourceSize.width
    }

    Text {
        anchors {
            centerIn: parent
        }
        z: 99

        color: Theme.primaryColor
        visible: friendmodel.unviewedMessages > 0
        font.bold: true
        text: qsTr("Messages: ") + friendmodel.unviewedMessages
    }

    CoverActionList {
        id: coverAction
        enabled: toxcore.status > 0

        CoverAction {
            iconSource: "../images/s" + (toxcore.status + 1 > 3 ? "1" : toxcore.status + 1) + ".png"
            onTriggered: {
                if ( toxcore.status === 0 ) return

                var s = toxcore.status + 1
                if ( s > 3 ) s = 1
                toxcore.status = s
            }
        }

        CoverAction {
            iconSource: "../images/s" + (toxcore.status + 2 > 3 ? toxcore.status + 2 - 3  : toxcore.status + 2) + ".png"
            onTriggered: {
                if ( toxcore.status === 0 ) return

                var s = toxcore.status + 2
                if ( s > 3 ) s = s - 3
                toxcore.status = s
            }
        }
    }
}

