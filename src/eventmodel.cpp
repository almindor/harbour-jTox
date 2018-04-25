#include "eventmodel.h"
#include "utils.h"
#include "friend.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QDebug>

namespace JTOX {

    EventModel::EventModel(ToxCore& toxCore, FriendModel& friendModel, DBData& dbData) : QAbstractListModel(0),
                    fToxCore(toxCore), fFriendModel(friendModel), fDBData(dbData),
                    fList(), fTimerViewed(), fTimerTyping(), fFriendID(-1), fTyping(false)
    {
        connect(&toxCore, &ToxCore::messageDelivered, this, &EventModel::onMessageDelivered);
        connect(&toxCore, &ToxCore::messageReceived, this, &EventModel::onMessageReceived);
        connect(&toxCore, &ToxCore::fileReceived, this, &EventModel::onFileReceived);
        connect(&toxCore, &ToxCore::fileCanceled, this, &EventModel::onFileCanceled);
        connect(&toxCore, &ToxCore::filePaused, this, &EventModel::onFilePaused);
        connect(&toxCore, &ToxCore::fileResumed, this, &EventModel::onFileResumed);
        connect(&toxCore, &ToxCore::fileChunkReceived, this, &EventModel::onFileChunkReceived);
        connect(&friendModel, &FriendModel::friendUpdated, this, &EventModel::onFriendUpdated);
        connect(&friendModel, &FriendModel::friendWentOnline, this, &EventModel::onFriendWentOnline);
        connect(&fTimerViewed, &QTimer::timeout, this, &EventModel::onMessagesViewed);
        connect(&fTimerTyping, &QTimer::timeout, this, &EventModel::onTypingDone);

        fTimerViewed.setInterval(2000); // 2 sec after viewing we consider msgs read TODO: combine with actually viewed msgs from QML
        fTimerViewed.setSingleShot(true);
        fTimerTyping.setInterval(2000);
        fTimerTyping.setSingleShot(true);
    }

    EventModel::~EventModel() {
        cancelTransfers();
        fDB.close();
    }

    QHash<int, QByteArray> EventModel::roleNames() const {
        QHash<int, QByteArray> result;
        result[erID] = "event_id";
        result[erEventType] = "event_type";
        result[erCreated] = "created_at";
        result[erMessage] = "message";
        result[erFileSize] = "file_size";
        result[erFileName] = "file_name";

        return result;
    }

    int EventModel::rowCount(const QModelIndex &parent) const {
        Q_UNUSED(parent);
        return fList.size();
    }

    QVariant EventModel::data(const QModelIndex &index, int role) const {
        int row = index.row();
        if ( row < 0 || row >= fList.size() ) {
            Utils::bail("Requesting out of bounds data");
        }

        return fList.at(row).value(role);
    }

    int EventModel::getFriendID() const
    {
        return fFriendID;
    }

    qint64 EventModel::sendMessageRaw(const QString& message, qint64 friendID, int id)
    {
        qint64 sendID = -1;
        const QByteArray rawMsg = message.trimmed().toUtf8();
        TOX_ERR_FRIEND_SEND_MESSAGE error;
        sendID = tox_friend_send_message(fToxCore.tox(), friendID, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*) rawMsg.data(),
                                         rawMsg.size(), &error);
        if ( !handleSendMessageError(error) ) {
            return -1;
        }

        fDBData.updateEventSent(id, etMessageOutPending, sendID);
        return sendID;
    }

    qint64 EventModel::setFriendIndex(int friendIndex)
    {
        setFriend(fFriendModel.getFriendIDByIndex(friendIndex));
        return fFriendID;
    }

    void EventModel::setFriend(qint64 friendID) {
        if ( fFriendID == friendID ) {
            return;
        }

        setTyping(false);
        fFriendID = friendID;

        if ( fFriendID < 0 ) {
            fTimerViewed.stop(); // make sure we don't try to mark someone else's messages by accident
            return;
        }

        beginResetModel();
        fDBData.getEvents(fList, fFriendID);
        endResetModel();

        emit friendUpdated();
        fTimerViewed.start();
    }

    void EventModel::sendMessage(const QString& message) {
        if ( !fToxCore.getInitialized() ) {
            Utils::bail("Send called when toxcore not initialized!");
        }

        int count = rowCount();
        EventType eventType = etMessageOutOffline; // default to offline msg
        qint64 sendID = -1;
        QDateTime createdAt;
        int id = fDBData.insertEvent(sendID, fFriendID, eventType, message, createdAt);

        if ( getFriendStatus() > 0 ) { // if friend is online, send it and use out pending mt
            sendID = sendMessageRaw(message, fFriendID, id);
            eventType = etMessageOutPending; // if we got here the message is out
        }

        beginInsertRows(QModelIndex(), count, count);
        fList.append(Event(id, fFriendID, createdAt, eventType, message, sendID));
        endInsertRows();
    }

    void EventModel::deleteMessage(int eventID)
    {
        int index = indexForEvent(eventID);
        if ( fList.at(index).type() != etMessageOutOffline ) {
            Utils::bail("Unable to delete online message");
        }

        beginRemoveRows(QModelIndex(), index, index);
        fDBData.deleteEvent(fList.at(index).id());
        fList.removeAt(index);
        endRemoveRows();
    }

    bool EventModel::fileExists(int eventID)
    {
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) || !transfer.isFile() ) {
            emit transferError("Unable to find file transfer in DB");
            return false;
        }

        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        QFile file(dir.absoluteFilePath(transfer.fileName()));
        return file.exists();
    }

    bool EventModel::deleteFile(int eventID)
    {
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) || !transfer.isFile()) {
            emit transferError("Unable to find file transfer in DB");
            return false;
        }

        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        QFile file(dir.absoluteFilePath(transfer.fileName()));
        if ( !file.remove() ) {
            emit transferError("Unable to delete existing file");
            return false;
        }

        return true;
    }

    void EventModel::pauseFile(int eventID)
    {
        int index = indexForEvent(eventID);
        if ( fList.at(index).type() != etFileTransferInRunning && fList.at(index).type() != etFileTransferOutRunning ) {
            Utils::bail("Unable to pause file, invalid event");
        }

        // TODO: notify of busy less
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) ) {
            emit transferError("Unable to find file transfer in DB");
            return;
        }
        TOX_ERR_FILE_CONTROL error;
        tox_file_control(fToxCore.tox(), transfer.friendID(), transfer.sendID(), TOX_FILE_CONTROL_PAUSE, &error);
        if ( !handleFileControlError(error) ) {
            emit transferError("Unable to pause file transfer");
            return;
        }

        const EventType pausedType = transfer.type() == etFileTransferInRunning ? etFileTransferInPaused : etFileTransferOutPaused;
        updateEventType(transfer, pausedType);
    }

    void EventModel::resumeFile(int eventID)
    {
        int index = indexForEvent(eventID);
        if ( fList.at(index).type() != etFileTransferIn && fList.at(index).type() != etFileTransferInPaused ) {
            Utils::bail("Unable to resume file, invalid event");
        }

        // TODO: notify of busy need
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) ) {
            emit transferError("Transfer not found");
        }
        TOX_ERR_FILE_CONTROL error;
        tox_file_control(fToxCore.tox(), transfer.friendID(), transfer.sendID(), TOX_FILE_CONTROL_RESUME, &error);
        if ( !handleFileControlError(error) ) {
            emit transferError("Unable to resume file transfer");
            return;
        }

        updateEventType(transfer, etFileTransferInRunning);
    }

    void EventModel::cancelFile(int eventID)
    {
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) ) {
            emit transferError("Transfer not found");
            return;
        }

        cancelTransfer(transfer);
    }

    void EventModel::onMessageDelivered(quint32 friendID, quint32 sendID) {
        fDBData.deliverEvent(sendID, friendID);

        // if we're "open" on the given friend, make sure to update the UI
        if ( fFriendID == friendID ) {
            int row = 0;
            foreach ( const Event event, fList ) {
                if ( event.sendID() >= 0 && event.sendID() == sendID && event.type() == etMessageOutPending ) {
                    fList[row].delivered();
                    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>(1, erEventType));
                    break;
                }
                row++;
            }
        }
    }

    void EventModel::onMessageReceived(quint32 friend_id, TOX_MESSAGE_TYPE type, const QString &message)
    {
        if ( type != TOX_MESSAGE_TYPE_NORMAL ) {
            qDebug() << "TODO: Non normal message received\n"; // TODO: ??
            return;
        }

        QDateTime createdAt;
        int count = rowCount();
        bool activeFriend = fFriendID == friend_id && fToxCore.getApplicationActive();
        EventType eventType = activeFriend ? etMessageIn : etMessageInUnread;
        int id = fDBData.insertEvent(-1, friend_id, eventType, message, createdAt);

        if ( fFriendID == friend_id ) { // only read last msg if we're open on this
            beginInsertRows(QModelIndex(), count, count);
            fList.append(Event(id, friend_id, createdAt, eventType, message, -1));
            endInsertRows();
        }

        // only notify in case we're not open on sender already OR if we're in background regardless of which friend
        if ( !activeFriend ) {
            fFriendModel.unviewedMessageReceived(friend_id);
            emit messageReceived(fFriendModel.getListIndexForFriendID(friend_id), fFriendModel.getFriendByID(friend_id).name());
        }
    }

    void EventModel::onFriendUpdated(quint32 friend_id)
    {
        if ( friend_id != fFriendID ) {
            return;
        }

        emit friendUpdated();
    }

    void EventModel::onFriendWentOnline(quint32 friendID)
    {
        EventList offlineMessages;
        EventList pendingMessages; // there's a chance we sent it out, it never arrived and got stuck in state pending
        fDBData.getEvents(offlineMessages, friendID, etMessageOutOffline);
        fDBData.getEvents(pendingMessages, friendID, etMessageOutPending);

        offlineMessages.append(pendingMessages);

        foreach ( const Event& event, offlineMessages ) {
            qint64 sendID = sendMessageRaw(event.message(), friendID, event.id());

            if ( fFriendID == friendID ) { // we need to update list entries if we're active on given friend
                for ( int i = 0; i < fList.size(); i++ ) {
                    if ( fList.at(i).id() == event.id() ) {
                        fList[i].setSendID(sendID);
                        fList[i].setEventType(etMessageOutPending);
                    }
                }
            }
        }
    }

    void EventModel::onFileReceived(quint32 friend_id, quint32 file_id, quint64 file_size, const QString &file_name)
    {
        QDateTime createdAt;
        int count = rowCount();
        bool activeFriend = fFriendID == friend_id && fToxCore.getApplicationActive();
        const QString fileInfo = Utils::getFileInfo(file_size, file_name);
        int id = fDBData.insertEvent(file_id, friend_id, etFileTransferIn, fileInfo, createdAt);

        if ( fFriendID == friend_id ) { // add event to visible list if we're open on this friend
            beginInsertRows(QModelIndex(), count, count);
            fList.append(Event(id, friend_id, createdAt, etFileTransferIn, fileInfo, file_id));
            endInsertRows();
        }

        // only notify in case we're not open on sender already OR if we're in background regardless of which friend
        if ( !activeFriend ) {
            fFriendModel.unviewedMessageReceived(friend_id);
            emit messageReceived(fFriendModel.getListIndexForFriendID(friend_id), fFriendModel.getFriendByID(friend_id).name());
        }
    }

    void EventModel::onFileChunkReceived(quint32 friend_id, quint32 file_id, quint64 position, const QByteArray &data)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_id, etFileTransferInRunning, transfer) ) {
            emit transferError("Transfer not found");
            return;
        }

        if ( data.size() == 0 ) { // done
            return completeTransfer(transfer);
        }

        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        QFile file(dir.absoluteFilePath(transfer.fileName()));
        if ( !file.open(QIODevice::Append) ) {
            emit transferError("Error opening file for appending: " + file.errorString());
            return;
        }

        if ( (quint64)file.size() != position ) {
            emit transferError("Transfer file position mismatch");
            cancelTransfer(transfer);
            return;
        }

        file.write(data);
        file.close();
    }

    void EventModel::onFileCanceled(quint32 friend_id, quint32 file_id)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_id, etFileTransferInRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_id, etFileTransferInPaused, transfer) ) {
            Utils::bail("Unable to find transfer", true); // minor, don't crash on possibly race conditioned request
            return;
        }

        updateEventType(transfer, etFileTransferInCanceled);
    }

    void EventModel::onFilePaused(quint32 friend_id, quint32 file_id)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_id, etFileTransferInRunning, transfer) ) {
            Utils::bail("Unable to find transfer", true); // minor, don't crash on possibly race conditioned request
            return;
        }

        updateEventType(transfer, etFileTransferInPaused);
    }

    void EventModel::onFileResumed(quint32 friend_id, quint32 file_id)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_id, etFileTransferInPaused, transfer) ) {
            Utils::bail("Unable to find transfer", true); // minor, don't crash on possibly race conditioned request
            return;
        }

        updateEventType(transfer, etFileTransferInRunning);
    }

    int EventModel::indexForEvent(int eventID) const
    {
        for ( int i = 0; i < fList.size(); i++ ) {
            if ( fList.at(i).id() == eventID) {
                return i;
            }
        }

        Utils::bail("Could not find index for event ID: " + eventID);
        return -1;
    }

    bool EventModel::handleSendMessageError(TOX_ERR_FRIEND_SEND_MESSAGE error) const
    {
        switch ( error ) {
            case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY: return Utils::bail("Cannot send empty message");
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED: return Utils::bail("Cannot send message, friend not online");
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND: return Utils::bail("Cannot send message, friend not found");
            case TOX_ERR_FRIEND_SEND_MESSAGE_NULL: return Utils::bail("Cannot send null message");
            case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ: return Utils::bail("Cannot send message, sendq error");
            case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG: return Utils::bail("Cannot send message it is too long");
            case TOX_ERR_FRIEND_SEND_MESSAGE_OK: return true;
        }

        Utils::bail("Unknown error");
        return false;
    }

    bool EventModel::handleFileControlError(TOX_ERR_FILE_CONTROL error, bool soft) const
    {
        switch ( error ) {
            case TOX_ERR_FILE_CONTROL_ALREADY_PAUSED: return Utils::bail("File transfer already paused", soft);
            case TOX_ERR_FILE_CONTROL_DENIED: return Utils::bail("Permission denied", soft);
            case TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED: return Utils::bail("Friend not connected", soft);
            case TOX_ERR_FILE_CONTROL_FRIEND_NOT_FOUND: return Utils::bail("Friend not found", soft);
            case TOX_ERR_FILE_CONTROL_NOT_FOUND: return Utils::bail("File transfer not found", soft);
            case TOX_ERR_FILE_CONTROL_NOT_PAUSED: return Utils::bail("File transfer not paused", soft);
            case TOX_ERR_FILE_CONTROL_SENDQ: return Utils::bail("File transfer send queue full", soft);
            case TOX_ERR_FILE_CONTROL_OK: return true;
        }

        Utils::bail("Unknown error");
        return false;
    }

    int EventModel::getFriendStatus() const {
        if ( fFriendID < 0 ) {
            return 0;
        }

        return fFriendModel.getFriendByID(fFriendID).status();
    }

    bool EventModel::getFriendTyping() const {
        if ( fFriendID < 0 ) {
            return false;
        }

        return fFriendModel.getFriendByID(fFriendID).typing();
    }

    const QString EventModel::getFriendName() const {
        if ( fFriendID < 0 ) {
            return QString();
        }

        return fFriendModel.getFriendByID(fFriendID).name();
    }


    bool EventModel::getTyping() const {
        return fTyping;
    }

    void EventModel::setTyping(bool typing) {
        setTyping(fFriendID, typing);
    }

    void EventModel::setTyping(qint64 friendID, bool typing)
    {
        if ( friendID < 0 ) {
            return;
        }

        fTimerTyping.stop();
        if ( typing ) {
            fTimerTyping.start();
        }

        if ( typing == fTyping ) {
            return;
        }

        TOX_ERR_SET_TYPING error;
        tox_self_set_typing(fToxCore.tox(), friendID, typing, &error);
        if ( error != TOX_ERR_SET_TYPING_OK ) {
            Utils::bail("Friend not found");
        }

        fTyping = typing;
        emit typingChanged(fTyping);
    }

    void EventModel::cancelTransfer(const Event &transfer)
    {
        EventType canceledType = etFileTransferInCanceled;
        switch ( transfer.type() ) {
            case etFileTransferIn:
            case etFileTransferInPaused:
            case etFileTransferInRunning: canceledType = etFileTransferInCanceled; break;
            case etFileTransferOut:
            case etFileTransferOutPaused:
            case etFileTransferOutRunning: canceledType = etFileTransferOutCanceled; break;
            default: Utils::bail("Unable to cancel transfer, invalid event"); break;
        }

        TOX_ERR_FILE_CONTROL error;
        tox_file_control(fToxCore.tox(), transfer.friendID(), transfer.sendID(), TOX_FILE_CONTROL_CANCEL, &error);

        if ( handleFileControlError(error, true) ) { // don't fail on cancel, just log. friend could be off etc.
            updateEventType(transfer, canceledType);
        }
    }

    void EventModel::cancelTransfers()
    {
        // cancel all transfers, in progress, paused or pending
        EventList transfers;
        fDBData.getTransfers(transfers);

        foreach ( const Event& transfer, transfers ) {
            cancelTransfer(transfer);
        }
    }

    void EventModel::completeTransfer(const Event &transfer)
    {
        EventType eType = transfer.type() == etFileTransferInRunning ? etFileTransferInDone : etFileTransferOutDone;
        updateEventType(transfer, eType);
        emit transferComplete(transfer.fileName(), fFriendModel.getListIndexForFriendID(transfer.friendID()), fFriendModel.getFriendByID(transfer.friendID()).name());
    }

    void EventModel::updateEventType(const Event &event, EventType eventType)
    {
        fDBData.updateEventType(event.id(), eventType);

        int index = -1;
        if ( fFriendID == event.friendID() && (index = indexForEvent(event.id())) >= 0 ) {
            fList[index].setEventType(eventType);
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>(1, erEventType));
        }
    }

    void EventModel::onMessagesViewed()
    {
        if ( fFriendID < 0 ) return; // shouldn't happen as we clear the timer on setFriend(-1), but just in case

        int row = 0;
        int first = -1, last = -1;
        foreach ( const Event event, fList ) {
            if ( event.type() == etMessageInUnread ) {
                if ( first < 0 ) first = row;
                if ( row > last ) last = row;
                fDBData.updateEventType(fList.at(row).id(), etMessageIn);
                fList[row].viewed();
            }
            row++;
        }
        emit dataChanged(createIndex(first, 0), createIndex(last, 0), QVector<int>(1, erEventType));
        fFriendModel.messagesViewed(fFriendID);
    }

    void EventModel::onTypingDone()
    {
        setTyping(fFriendID, false);
    }

}
