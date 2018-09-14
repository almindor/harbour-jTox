import QtQuick 2.0
import Sailfish.Silica 1.0

Image {
    property int source_id : -1
    property string placeholder: ""

    source: "image://avatarProvider/" + source_id + "?id=" + Math.random() // reload on signals
    asynchronous: true
    cache: false

    function refresh() {
        source = "image://avatarProvider/" + source_id + "?id=" + Math.random() // reload on signals
    }

    Connections {
        target: avatarProvider
        onAvatarChanged: {
            if ( friend_id === source_id ) {
                refresh()
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: parent.sourceSize.width <= 1 && placeholder.length > 0 // 1 or less means empty/unknown
        font.pixelSize: parent.height
        color: Theme.highlightColor
        text: placeholder
    }
}
