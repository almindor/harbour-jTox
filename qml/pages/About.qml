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

// debug
import QtMultimedia 5.0

Page {
    id: page
    allowedOrientations: Orientation.Portrait

    MediaPlayer {
        source: "file:///home/nemo/test.wav"
        autoPlay: true
    }

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: col.height

        VerticalScrollDecorator {
            flickable: parent
        }

        Column {
            id: col
            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                id: header
                title: qsTr("About")
            }

            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                wrapMode: Text.WordWrap
                color: Theme.primaryColor
                font.bold: true
                text: qsTr("jTox") + " v1.3.3 " + qsTr("by Aleš Katona.")
            }

            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                text: qsTr("Toxcore version") + " " + toxcore.majorVersion + "." + toxcore.minorVersion + "." + toxcore.patchVersion
            }

            Text {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                color: Theme.secondaryColor
                font.pointSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WordWrap
                text: qsTr("jTox is free software licensed under the GPL v3 available at <a href='https://github.com/almindor/harbour-jTox'>github</a>.")
                linkColor: Theme.secondaryHighlightColor
                onLinkActivated: {
                    Qt.openUrlExternally(link);
                }
            }

            Text {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                color: Theme.secondaryColor
                font.pointSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WordWrap
                text: "<b>" + qsTr("Contributors") + "</b><br>" + qsTr("Michal Szczepaniak")
            }

            Text {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                color: Theme.secondaryColor
                font.pointSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WordWrap
                text: "<b>" + qsTr("Translators") + "</b><br>" + qsTr("Swedish: Åke Engelbrektson") + "<br>" + qsTr("Spanish: carlosgonz") + "<br>" + qsTr("French: Jerome Hubert")
            }

            Text {
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: Theme.paddingLarge
                }
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeMedium
                wrapMode: Text.WordWrap
                text: qsTr("The Tox protocol is <b>EXPERIMENTAL!</b> The protocol has not been audited for security and can contain vulnerabilities. Large data usage is to be expected. <b>USE AT YOUR OWN RISK</b>.")
            }
        }
    }
}
