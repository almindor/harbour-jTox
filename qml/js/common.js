
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
    FileTransferOutDone: 19
};

function isMessage(et) {
    return [EventType.MessageIn, EventType.MessageInUnread, EventType.MessageOut, EventType.MessageOutPending, EventType.MessageOutOffline].indexOf(et) >= 0;
}

function isFile(et) {
    return [EventType.FileTransferIn, EventType.FileTransferOut,
            EventType.FileTransferInPaused, EventType.FileTransferOutPaused,
            EventType.FileTransferInCanceled, EventType.FileTransferOutCanceled,
            EventType.FileTransferInRunning, EventType.FileTransferOutRunning,
            EventType.FileTransferInDone, EventType.FileTransferOutDone].indexOf(et) >= 0;
}

function isFilePending(et) {
    return [EventType.FileTransferIn, EventType.FileTransferOut, EventType.FileTransferInPaused, EventType.FileTransferOutPaused].indexOf(et) >= 0;
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

function isMessageIncoming(et) {
    return [EventType.MessageIn, EventType.MessageInUnread, EventType.FileTransferIn,
            EventType.FileTransferInPaused, EventType.FileTransferInCanceled,
            EventType.FileTransferInRunning, EventType.FileTransferInDone].indexOf(et) >= 0;
}

function isMessageOutgoing(et) {
    return [EventType.MessageOut, EventType.MessageOutPending, EventType.MessageOutOffline,
            EventType.FileTransferOut, EventType.FileTransferOutPaused,
            EventType.FileTransferOutCanceled, EventType.FileTransferOutRunning, EventType.FileTransferOutDone].indexOf(et) >= 0;
}

function isMessagePending(et) {
    return [EventType.MessageOutPending, EventType.MessageOutOffline].indexOf(et) >= 0;
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
