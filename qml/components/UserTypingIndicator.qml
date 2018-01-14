import QtQuick 2.0
import Sailfish.Silica 1.0

Item {    
    width: Theme.itemSizeExtraSmall * 0.5
    height: Theme.itemSizeExtraSmall * 0.5
    Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingSmall

        Repeater {
            model: 3

            Rectangle {
                id: dot

                NumberAnimation on opacity {
                    running: visible
                    from: 1.0 / parseFloat(index + 2)
                    to: 1.0
                    duration: 1000 / (3 - index)
                    onStopped: repeatedAnimation.start()
                }

                SequentialAnimation on opacity {
                    id: repeatedAnimation
                    running: false
                    loops: Animation.Infinite

                    NumberAnimation {
                        from: 1.0
                        to: 0.5
                        duration: 1000
                    }
                    NumberAnimation {
                        from: 0.5
                        to: 1.0
                        duration: 1000
                    }
                }

                opacity: 1.0 / parseFloat(index + 2)
                width: 8
                height: 8
                color: Theme.highlightColor
                border.color: Theme.highlightDimmerColor
                border.width: 2
                radius: width * 0.5
            }
        }
    }


}
