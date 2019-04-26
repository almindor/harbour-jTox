
.pragma library

var EventType = {
    Edit: 0,
    MessageOut: 1,
    MessageInUnread: 2,
    MessageOutPending: 3,
    MessageIn: 4,
    MessageOutOffline: 5, // pending to be sent
    CallOutRejected: 6,
    CallOutAccepted: 7,
    CallInRejected: 8,
    CallInAccepted: 9,
    FileTransferIn: 10,
    FileTransferOut: 11,
    FileTransferInPaused: 12,
    FileTransferOutPaused: 13,
    FileTransferInCanceled: 14,
    FileTransferOutCanceled: 15,
    FileTransferInRunning: 16,
    FileTransferOutRunning: 17,
    FileTransferInDone: 18,
    FileTransferOutDone: 19,
    CallInPending: 20,
    CallOutPending: 21,
    CallInFinished: 22,
    CallOutFinished: 23,
    CallInMissed: 24,
    CallOutMissed: 25
};

function jumpToCall(appWindow, pageStack, eventmodel, friend_id, friendIndex, animType) {
    if ( !appWindow.applicationActive ) {
        appWindow.activate()
    }

    // jump up to friends page
    while ( pageStack.depth > 1 ) {
        pageStack.pop(null, animType)
    }

    if (friendIndex >= 0) {
        friend_id = eventmodel.setFriendIndex(friendIndex)
    }

    appWindow.activeFriendID = friend_id
    eventmodel.setFriend(friend_id)
    pageStack.push("../pages/Messages.qml", { friend_id: appWindow.activeFriendID }, animType)
    pageStack.pushAttached("../pages/Calls.qml", { friend: appWindow.activeFriendID }, animType)
    pageStack.navigateForward(animType)
}

function isMessage(et) {
    return [EventType.MessageIn, EventType.MessageInUnread, EventType.MessageOut,
            EventType.MessageOutPending, EventType.MessageOutOffline].indexOf(et) >= 0;
}

function isCall(et) {
    return [EventType.CallInPending, EventType.CallOutPending,
            EventType.CallInAccepted, EventType.CallOutAccepted,
            EventType.CallInRejected, EventType.CallOutRejected,
            EventType.CallInFinished, EventType.CallOutFinished,
            EventType.CallInMissed, EventType.CallOutMissed].indexOf(et) >= 0;
}

function isCallPending(et) {
    return [EventType.CallInPending, EventType.CallOutPending].indexOf(et) >= 0;
}

function isCallRejected(et) {
    return [EventType.CallInRejected, EventType.CallOutRejected,
            EventType.CallInMissed, EventType.CallOutMissed].indexOf(et) >= 0;
}

function toTimeStr(msSinceEpoch) {
    if (!msSinceEpoch || msSinceEpoch < 0) {
        return '00:00:00?';
    }

    var date = new Date(null)
    date.setSeconds(msSinceEpoch / 1000)
    return date.toISOString().substr(11, 8)
}

function msgForCall(et, duration) {
    switch (et) {
        case EventType.CallInPending:
        case EventType.CallInAccepted: return qsTr('Incoming call')
        case EventType.CallOutPending:
        case EventType.CallOutAccepted: return qsTr('Outgoing call')
        case EventType.CallInRejected: return qsTr('Rejected call')
        case EventType.CallOutRejected: return qsTr('Declined call')
        case EventType.CallInMissed: return qsTr('Missed call')
        case EventType.CallOutMissed: return qsTr('Unanswered call')
        case EventType.CallInFinished:
        case EventType.CallOutFinished: return qsTr('Finished call') + ' (' + toTimeStr(duration) + ')'
    }

    return qsTr('Unknown call state')
}

function isFile(et) {
    return [EventType.FileTransferIn, EventType.FileTransferOut,
            EventType.FileTransferInPaused, EventType.FileTransferOutPaused,
            EventType.FileTransferInCanceled, EventType.FileTransferOutCanceled,
            EventType.FileTransferInRunning, EventType.FileTransferOutRunning,
            EventType.FileTransferInDone, EventType.FileTransferOutDone].indexOf(et) >= 0;
}

function isFilePending(et) {
    return [EventType.FileTransferIn, EventType.FileTransferOut,
            EventType.FileTransferInPaused, EventType.FileTransferOutPaused].indexOf(et) >= 0;
}

function isFileCanceled(et) {
    return [EventType.FileTransferInCanceled, EventType.FileTransferOutCanceled].indexOf(et) >= 0;
}

function isFilePaused(et) {
    return [EventType.FileTransferInPaused, EventType.FileTransferOutPaused].indexOf(et) >= 0;
}

function isFileRunning(et) {
    return [EventType.FileTransferInRunning, EventType.FileTransferOutRunning].indexOf(et) >= 0;
}

function isFileDone(et) {
    return [EventType.FileTransferInDone, EventType.FileTransferOutDone].indexOf(et) >= 0;
}

function isFileActive(et) {
    return isFilePending(et) || isFileRunning(et);
}

function isEventIncoming(et) {
    return [EventType.MessageIn, EventType.MessageInUnread, EventType.FileTransferIn,
            EventType.FileTransferInPaused, EventType.FileTransferInCanceled,
            EventType.FileTransferInRunning, EventType.FileTransferInDone,
            EventType.CallInPending, EventType.CallInAccepted, EventType.CallInRejected,
            EventType.CallInFinished, EventType.CallInMissed].indexOf(et) >= 0;
}

function isEventOutgoing(et) {
    return [EventType.MessageOut, EventType.MessageOutPending, EventType.MessageOutOffline,
            EventType.FileTransferOut, EventType.FileTransferOutPaused,
            EventType.FileTransferOutCanceled, EventType.FileTransferOutRunning, EventType.FileTransferOutDone,
            EventType.CallOutPending, EventType.CallOutAccepted, EventType.CallOutRejected,
            EventType.CallOutFinished, EventType.CallOutMissed].indexOf(et) >= 0;
}

function isMessagePending(et) {
    return [EventType.MessageOutPending, EventType.MessageOutOffline].indexOf(et) >= 0;
}

function isEventPending(et) {
    return isMessagePending(et) || isFilePending(et) || isCallPending(et)
}

function wrapMessage(message, et, file_size) {
    if (isFile(et)) {
        var hfs = humanFileSize(file_size, true)
        return (message + " (" +  + ")")
    }

    return message;
}

function humanFileSize(bytes, si) {
    var thresh = si ? 1000 : 1024;
    if(Math.abs(bytes) < thresh) {
        return bytes + ' B';
    }
    var units = si
        ? ['kB','MB','GB','TB','PB','EB','ZB','YB']
        : ['KiB','MiB','GiB','TiB','PiB','EiB','ZiB','YiB'];
    var u = -1;
    do {
        bytes /= thresh;
        ++u;
    } while(Math.abs(bytes) >= thresh && u < units.length - 1);
    return bytes.toFixed(1)+' '+units[u];
}

function prettifyDateTime(datetime) {
    var prettyDateTime = "";
    var now = new Date();

    if ( parseInt(Qt.formatDateTime(datetime, "d")) < parseInt(Qt.formatDateTime(now, "d")) ) {
        if (parseInt(Qt.formatDateTime(datetime, "yyyy")) < parseInt(Qt.formatDateTime(now, "yyyy"))) {
            prettyDateTime = Qt.formatDateTime(datetime, "dd-MM-yy hh:mm");
        } else {
            prettyDateTime = Qt.formatDateTime(datetime, "dd-MMM hh:mm");
        }
    } else {
        prettyDateTime = Qt.formatTime(datetime, "hh:mm");
    }

    return prettyDateTime;
}

function qmlForEventType(et) {
    if (isMessage(et)) {
        return "MessageItemText.qml";
    }

    if (isFile(et)) {
        return "MessageItemFile.qml";
    }

    if (isCall(et)) {
        return "MessageItemCall.qml";
    }

    console.error("Unknown category: " + et);
    return undefined;
}
