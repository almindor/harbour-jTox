import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    width: 32
    height: 32
    property int userStatus
    property bool offlineBusy: false

    // show when offlineBusy is true and offline
    BusyIndicator {
        visible: offlineBusy && userStatus === 0
        running: offlineBusy && userStatus === 0
        size: BusyIndicatorSize.Small
    }

    // show when offlineBusy is false or when not offline
    Rectangle {
        visible: !offlineBusy || userStatus > 0
        anchors.fill: parent
        color: userStatus == 0 ? "transparent" : (userStatus == 1 ? Theme.highlightColor : Theme.secondaryHighlightColor)
        border.color: Theme.highlightDimmerColor
        border.width: 2
        radius: width * 0.5

        // when busy
        Rectangle {
            visible: userStatus == 3
            width: parent.width * 0.66
            height: parent.height * 0.1
            anchors {
                centerIn: parent
            }
            color: Theme.highlightColor
        }
    }
}
