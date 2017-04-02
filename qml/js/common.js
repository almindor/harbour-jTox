
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
    FileTransfer: 10
};

function isMessageIncoming(et) {
    return [EventType.MessageIn, EventType.MessageInUnread].indexOf(et) >= 0;
}

function isMessageOutgoing(et) {
    return [EventType.MessageOut, EventType.MessageOutPending, EventType.MessageOutOffline].indexOf(et) >= 0;
}

function isMessagePending(et) {
    return [EventType.MessageOutPending, EventType.MessageOutOffline].indexOf(et) >= 0;
}
