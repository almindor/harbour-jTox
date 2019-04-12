#include "eventmodel.h"
#include "utils.h"
#include "friend.h"
#include "toxcoreav.h"
#include <QDateTime>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace JTOX {

    qint64 sLastPositionUpdate = 0;

    EventModel::EventModel(ToxCore& toxCore, FriendModel& friendModel, DBData& dbData) : QAbstractListModel(0),
                    fToxCore(toxCore), fFriendModel(friendModel), fDBData(dbData),
                    fList(), fTimerViewed(), fTimerTyping(), fFriendID(-1), fTyping(false), fTransferFiles()
    {
        connect(&toxCore, &ToxCore::messageDelivered, this, &EventModel::onMessageDelivered);
        connect(&toxCore, &ToxCore::messageReceived, this, &EventModel::onMessageReceived);
        connect(&toxCore, &ToxCore::fileReceived, this, &EventModel::onFileReceived);
        connect(&toxCore, &ToxCore::fileCanceled, this, &EventModel::onFileCanceled);
        connect(&toxCore, &ToxCore::filePaused, this, &EventModel::onFilePaused);
        connect(&toxCore, &ToxCore::fileResumed, this, &EventModel::onFileResumed);
        connect(&toxCore, &ToxCore::fileChunkReceived, this, &EventModel::onFileChunkReceived);
        connect(&toxCore, &ToxCore::fileChunkRequest, this, &EventModel::onFileChunkRequest);
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
        result[erFileName] = "file_name";
        result[erFilePath] = "file_path";
        result[erFileID] = "file_id";
        result[erFileSize] = "file_size";
        result[erFilePosition] = "file_position";
        result[erFilePausers] = "file_pausers";

        return result;
    }

    int EventModel::rowCount(const QModelIndex &parent) const {
        Q_UNUSED(parent);
        return fList.size();
    }

    QVariant EventModel::data(const QModelIndex &index, int role) const {
        int row = index.row();
        if ( row < 0 || row >= fList.size() ) {
            Utils::fatal("Requesting out of bounds data");
        }

        return fList.at(row).value(role);
    }

    int EventModel::getFriendID() const
    {
        return fFriendID;
    }

    qint64 EventModel::sendMessageRaw(const QString& message, qint64 friendID, int id, QString& strError)
    {
        qint64 sendID = -1;
        const QByteArray rawMsg = message.toUtf8();
        if ( rawMsg.size() == 0 ) { // empty msg is disallowed in toxcore
            return sendID;
        }
        TOX_ERR_FRIEND_SEND_MESSAGE error;
        sendID = tox_friend_send_message(fToxCore.tox(), friendID, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*) rawMsg.data(),
                                         rawMsg.size(), &error);
        strError = Utils::handleSendMessageError(error, true);
        if ( !strError.isEmpty() ) {
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
            Utils::fatal("Send called when toxcore not initialized!");
        }

        if ( message.isEmpty() ) {
            emit eventError(tr("Cannot send empty message"));
            return; // this would cause error in toxcore!
        }

        StringListUTF8 parts = Utils::splitStringUTF8(message.toUtf8(), tox_max_message_length());

        foreach ( const QByteArray part, parts ) {
            EventType eventType = etMessageOutOffline; // default to offline msg
            qint64 sendID = -1;
            QDateTime createdAt;
            Event event(-1, fFriendID, createdAt, eventType, part, sendID);
            fDBData.insertEvent(event);

            if ( getFriendStatus() > 0 ) { // if friend is online, send it and use out pending mt
                QString strError;
                sendID = sendMessageRaw(part, fFriendID, event.id(), strError);
                if ( sendID < 0 ) { // shouldn't happen since we check input now, but we need to handle somehow
                    fDBData.deleteEvent(event.id());
                    emit eventError(strError);
                    return;
                }
                eventType = etMessageOutPending; // if we got here the message is out
                event.setEventType(eventType);
                event.setSendID(sendID);
            }

            beginInsertRows(QModelIndex(), 0, 0);
            fList.push_front(event);
            endInsertRows();
        }
    }

    void EventModel::deleteMessage(int eventID)
    {
        int index = indexForEvent(eventID);
        if ( fList.at(index).type() != etMessageOutOffline ) {
            Utils::fatal("Unable to delete online message");
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

    void EventModel::sendFile(const QString &filePath)
    {
        if ( fFriendID < 0 ) {
            emit transferError("Invalid friend id in event model");
            return;
        }
        const QFile file(filePath);
        if ( !file.exists() ) {
            emit transferError("File not found");
            return;
        }

        QByteArray fileID; // TODO: allow resume between restarts by setting this to existing file_id from db
        quint32 fileNumber = fToxCore.sendFile(fFriendID, filePath, fileID);

        QDateTime createdAt;
        Event event(-1, fFriendID, createdAt, etFileTransferOut, QFileInfo(file).fileName(), fileNumber, filePath, fileID, file.size(), 0, 0x2);
        fDBData.insertEvent(event);

        beginInsertRows(QModelIndex(), 0, 0);
        fList.push_front(event);
        endInsertRows();
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
        // TODO: notify of busy less
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) ) {
            emit transferError("Unable to find file transfer in DB");
            return;
        }

        EventType pausedType = etFileTransferInPaused;
        switch ( transfer.type() ) {
            case etFileTransferInRunning:
            case etFileTransferInPaused: pausedType = etFileTransferInPaused; break;
            case etFileTransferOutRunning:
            case etFileTransferOutPaused: pausedType = etFileTransferOutPaused; break;
            default: Utils::fatal("Unable to pause file, invalid event type"); return;
        }

        if ( (transfer.filePausers() & 0x1) != 0 ) {
            Utils::fatal("Unable to pause file, already paused locally");
        }

        TOX_ERR_FILE_CONTROL error;
        tox_file_control(fToxCore.tox(), transfer.friendID(), transfer.sendID(), TOX_FILE_CONTROL_PAUSE, &error);
        const QString strError = Utils::handleFileControlError(error);
        if ( !strError.isEmpty() ) {
            emit transferError("Unable to pause file transfer");
            return;
        }

        QVector<int> roles(2);
        roles[0] = erEventType;
        roles[1] = erFilePausers;
        updateEvent(transfer, pausedType, transfer.filePosition(), transfer.filePausers() | 0x1, roles); // 1st bit us 2nd bit them
    }

    void EventModel::resumeFile(int eventID)
    {
        // TODO: notify of busy need
        Event transfer;
        if ( !fDBData.getEvent(eventID, transfer) ) {
            emit transferError("Transfer not found");
            return;
        }

        if ( (transfer.filePausers() & 0x1) == 0x0 ) { // if not paused on our side
            Utils::fatal("Unable to resume file, not paused locally");
        }

        EventType resumeType = etFileTransferInRunning;
        switch ( transfer.type() ) {
            case etFileTransferIn: resumeType = etFileTransferInRunning; break;
            case etFileTransferInPaused: resumeType = etFileTransferInRunning; break;
            case etFileTransferOutPaused: resumeType = etFileTransferOutRunning; break;
            default: Utils::fatal("Unable to resume file, invalid event type"); return;
        }

        if ( (transfer.filePausers() & 0x2) != 0x0 ) { // if still paused on their side keep paused on event type
            resumeType = resumeType == etFileTransferInRunning ? etFileTransferInPaused : etFileTransferOutPaused;
        }

        TOX_ERR_FILE_CONTROL error;
        tox_file_control(fToxCore.tox(), transfer.friendID(), transfer.sendID(), TOX_FILE_CONTROL_RESUME, &error);
        const QString strError = Utils::handleFileControlError(error);
        if ( !strError.isEmpty() ) {
            emit transferError("Unable to resume file transfer");
            return;
        }

        QVector<int> roles(2);
        roles[0] = erEventType;
        roles[1] = erFilePausers;
        updateEvent(transfer, resumeType, transfer.filePosition(), transfer.filePausers() ^ 0x1, roles); // 1st bit us 2nd bit them
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

    void EventModel::refreshFilePosition(int index)
    {
        if ( index >= 0 && index < fList.size() ) {
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>(1, erFilePosition));
        }
    }

    void EventModel::acceptCall()
    {
        Event event;

        if (!fDBData.getCall(fFriendID, event)) {
            emit eventError("Unable to find previous call state");
            return;
        }

        if (event.type() != etCallInPending) {
            emit eventError("Unable to accept call: invalid call state");
            return;
        }

        emit acceptCall(); // eventmodel -> ToxCoreAV -> toxav_accept
    }

    void EventModel::onOutgoingCall(quint32 friend_id)
    {
        qDebug() << "[EventModel] onOutgoingCall(" << friend_id << ")\n";

        Event event(-1, friend_id, QDateTime(), etCallOutPending, QString(), -1);
        fDBData.insertEvent(event);

        if ( fFriendID == friend_id ) { // only show if we're open on friend
            beginInsertRows(QModelIndex(), 0, 0);
            fList.push_front(event);
            endInsertRows();
        }
    }

    void EventModel::onIncomingCall(quint32 friend_id, bool audio, bool video)
    {
        qDebug() << "[EventModel] onIncomingCall(" << friend_id << "," << audio << "," << video << ")\n";

        Event event(-1, friend_id, QDateTime(), etCallInPending, QString(), -1);
        fDBData.insertEvent(event);

        if ( fFriendID == friend_id ) { // only show if we're open on friend
            beginInsertRows(QModelIndex(), 0, 0);
            fList.push_front(event);
            endInsertRows();
        }

        emit incomingCall(fFriendModel.getListIndexForFriendID(friend_id), fFriendModel.getFriendByID(friend_id).name(), audio, video);
    }

    void EventModel::onCallStateChanged(quint32 friend_id, quint32 state)
    {
        qDebug() << "[EventModel] onCallStateChanged(" << friend_id << "," << state << ")\n";

        Event event;
        // try incoming first
        if (!fDBData.getCall(friend_id, event)) {
            emit eventError("Unable to find previous call state");
            return;
        }

        bool callIsIncoming = event.type() == etCallInAccepted || event.type() == etCallInPending || event.type() == etCallInRejected;
        bool callIsOngoing = event.type() == etCallInAccepted || event.type() == etCallOutAccepted;

        EventType newState;

        switch (state) {
            case csNone: {
                if (callIsIncoming) {
                    if (callIsOngoing) { // ongoing -> none == finished
                        newState = etCallInFinished;
                    } else {
                        newState = etCallInRejected; // ringing -> none = rejected
                    }
                } else {
                    if (callIsOngoing) { // ongoing -> none == finished
                        newState = etCallOutFinished;
                    } else {
                        newState = etCallOutRejected; // ringing -> none = rejected
                    }
                }
            } break;
            case csRinging: newState = callIsIncoming ? etCallInPending : etCallOutPending; break;
            case csActive: newState = callIsIncoming ? etCallInAccepted : etCallOutAccepted; break;
            default: {
                emit eventError("Invalid call state");
                return;
            }
        }

        if (event.type() != newState) {
            fDBData.updateEventType(event.id(), newState);

            if (fFriendID == friend_id) {
                int row = indexForEvent(event.id());
                emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>(1, erEventType));
            }
        }
    }

    void EventModel::onMessageDelivered(quint32 friendID, quint32 sendID) {
        fDBData.deliverEvent(sendID, friendID);

        // if we're "open" on the given friend, make sure to update the UI
        if ( fFriendID != friendID ) {
            return;
        }

        for ( int row = fList.size() - 1; row >= 0; row-- ) {
            const Event& event = fList.at(row);
            if ( event.sendID() >= 0 && event.sendID() == sendID && event.type() == etMessageOutPending ) {
                fList[row].delivered();
                emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>(1, erEventType));
                return;
            }
        }
    }

    void EventModel::onMessageReceived(quint32 friend_id, TOX_MESSAGE_TYPE type, const QString &message)
    {
        if ( type != TOX_MESSAGE_TYPE_NORMAL ) {
            return;
        }

        QDateTime createdAt;
        bool activeFriend = fFriendID == friend_id && fToxCore.getApplicationActive();
        EventType eventType = activeFriend ? etMessageIn : etMessageInUnread;
        Event event(-1, friend_id, createdAt, eventType, message, -1);
        fDBData.insertEvent(event);

        if ( fFriendID == friend_id ) { // only read last msg if we're open on this
            beginInsertRows(QModelIndex(), 0, 0);
            fList.push_front(event);
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

        int index = -1;
        foreach ( const Event& event, offlineMessages ) {
            QString strError;
            qint64 sendID = sendMessageRaw(event.message(), friendID, event.id(), strError);

            if ( sendID < 0 ) { // handled error case or empty message bug (fixed since)
                fDBData.deleteEvent(event.id()); // remove the message
                if ( fFriendID == friendID && (index = indexForEvent(event.id())) >= 0 ) {
                    beginRemoveRows(QModelIndex(), index, index);
                    fList.removeAt(index);
                    endRemoveRows();
                    emit eventError(tr("Removed invalid pending message"));
                    qDebug() << "removed invalid pending msg\n";
                }
                continue;
            }

            if ( fFriendID == friendID && (index = indexForEvent(event.id())) >= 0 ) {
                fList[index].setSendID(sendID);
                fList[index].setEventType(etMessageOutPending);
            }
        }
    }

    void EventModel::onFileReceived(quint32 friend_id, quint32 file_number, quint64 file_size, const QString &file_name)
    {
        QDateTime createdAt;
        bool activeFriend = fFriendID == friend_id && fToxCore.getApplicationActive();
        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
        const QString file_path = dir.absoluteFilePath(file_name);
        Event event(-1, friend_id, createdAt, etFileTransferIn, file_name, file_number, file_path, QByteArray(), file_size, 0, 0x1);
        fDBData.insertEvent(event);

        if ( fFriendID == friend_id ) { // add event to visible list if we're open on this friend
            beginInsertRows(QModelIndex(), 0, 0);
            fList.push_front(event);
            endInsertRows();
        }

        // only notify in case we're not open on sender already OR if we're in background regardless of which friend
        if ( !activeFriend ) {
            fFriendModel.unviewedMessageReceived(friend_id);
            emit transferReceived(fFriendModel.getListIndexForFriendID(friend_id), fFriendModel.getFriendByID(friend_id).name());
        }
    }

    void EventModel::onFileChunkReceived(quint32 friend_id, quint32 file_number, quint64 position, const QByteArray &data)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_number, etFileTransferInRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferInPaused, transfer) ) {
            // emit transferError("Transfer not found");
            // probably an avatar transfer, do nothing here
            return;
        }

        QFile* file = fileForTransfer(transfer, QIODevice::Append);
        if ( data.size() == 0 ) { // done
            return completeTransfer(transfer, position);
        }

        if ( (quint64) file->size() != position ) {
            emit transferError("Transfer file position mismatch");
            cancelTransfer(transfer);
            return;
        }

        file->write(data);

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if ( now - sLastPositionUpdate < 500 ) {
            return; // do not update db or in-mem too often, too expensive
        }
        sLastPositionUpdate = now;

        fDBData.updateEvent(transfer.id(), transfer.type(), position, transfer.filePausers());
        if ( fFriendID != transfer.friendID() ) {
            return; // not on active friend
        }

        int index = indexForEvent(transfer.id());
        if ( index >= 0 ) {
            fList[index].setFilePosition(position);
            // Do not emit, we query updates from "UI side" to prevent overload
        }
    }

    void EventModel::onFileChunkRequest(quint32 friend_id, quint32 file_number, quint64 position, size_t length)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_number, etFileTransferOut, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutPaused, transfer) ) {
            emit transferError("Transfer not found");
            return;
        }

        QFile* file = fileForTransfer(transfer, QIODevice::ReadOnly);

        if ( length == 0 ) { // done
            return completeTransfer(transfer, position);
        }

        if ( position >= (quint64) file->size() || !file->seek(position) ) {
            cancelTransfer(transfer);
            emit transferError("Transfer position invalid");
            return;
        }

        const QByteArray chunk = file->read(length);
        if ( (size_t) chunk.size() != length ) {
            cancelTransfer(transfer);
            emit transferError("Transfer chunk length invalid");
            return;
        }

        TOX_ERR_FILE_SEND_CHUNK error;
        tox_file_send_chunk(fToxCore.tox(), friend_id, file_number, position, (quint8*) chunk.constData(), length, &error);

        const QString strError = Utils::handleFileSendChunkError(error);
        if ( !strError.isEmpty() ) {
            cancelTransfer(transfer);
            emit transferError("Transfer chunk length invalid");
            return;
        }

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if ( now - sLastPositionUpdate < 500 ) {
            return; // do not update db or in-mem too often, too expensive
        }
        sLastPositionUpdate = now;

        fDBData.updateEvent(transfer.id(), transfer.type(), position + length, transfer.filePausers());
        // do not emit, UI will poll for updates

        if ( fFriendID == friend_id && position == 0 ) { // we just got "accepted" for sending
            onFileResumed(friend_id, file_number); // change to running and notify UI
        } else if ( fFriendID == friend_id ) {
            int index = indexForEvent(transfer.id());
            if ( index < 0 ) {
                emit transferError("Unable to find index for transfer"); // don't cancel tho
                return;
            }
            fList[index].setFilePosition(position + length);
        }
    }

    void EventModel::onFileCanceled(quint32 friend_id, quint32 file_number)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_number, etFileTransferInRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferInPaused, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferIn, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutPaused, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOut, transfer) ) {
            Utils::warn("Unable to find transfer"); // minor, don't crash on possibly race conditioned request
            return;
        }

        EventType cancelType = transfer.isIncoming() ? etFileTransferInCanceled : etFileTransferOutCanceled;
        updateEventType(transfer, cancelType);
        emit transferError(transfer.isIncoming() ? tr("Transfer canceled by sender") : tr("Transfer canceled by receiver"));
    }

    void EventModel::onFilePaused(quint32 friend_id, quint32 file_number)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_number, etFileTransferInRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferInPaused, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutPaused, transfer) ) {
            Utils::warn("Unable to find transfer"); // minor, don't crash on possibly race conditioned request
            return;
        }

        EventType pauseType = transfer.isIncoming() ? etFileTransferInPaused : etFileTransferOutPaused;
        QVector<int> roles(2);
        roles[0] = erEventType;
        roles[1] = erFilePausers;
        updateEvent(transfer, pauseType, transfer.filePosition(), transfer.filePausers() | 0x2, roles); // 1st bit for us 2nd bit for them
    }

    void EventModel::onFileResumed(quint32 friend_id, quint32 file_number)
    {
        Event transfer;
        if ( !fDBData.getEvent(friend_id, file_number, etFileTransferInRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferInPaused, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutRunning, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOutPaused, transfer) &&
             !fDBData.getEvent(friend_id, file_number, etFileTransferOut, transfer) ) {
            Utils::warn("Unable to find transfer"); // minor, don't crash on possibly race conditioned request
            return;
        }

        EventType resumeType = transfer.isIncoming() ? etFileTransferInRunning : etFileTransferOutRunning;
        QVector<int> roles(2);
        roles[0] = erEventType;
        roles[1] = erFilePausers;
        updateEvent(transfer, resumeType, transfer.filePosition(), transfer.filePausers() ^ 0x2, roles); // 1st bit for us 2nd bit for them
    }

    int EventModel::indexForEvent(int eventID) const
    {
        for ( int i = 0; i < fList.size(); i++ ) {
            if ( fList.at(i).id() == eventID) {
                return i;
            }
        }

        Utils::fatal("Cannot find index for event");
        return -1;
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
            Utils::fatal("Friend not found");
        }

        fTyping = typing;
        emit typingChanged(fTyping);
    }

    void EventModel::cancelTransfer(const Event &transfer)
    {
        quint64 transferID = Utils::transferID(transfer.friendID(), transfer.sendID());
        if ( fTransferFiles.contains(transferID) ) {
            fTransferFiles[transferID]->close();
            delete fTransferFiles[transferID];
            fTransferFiles.remove(transferID);
        }

        EventType canceledType = transfer.isIncoming() ? etFileTransferInCanceled : etFileTransferOutCanceled;

        TOX_ERR_FILE_CONTROL error;
        tox_file_control(fToxCore.tox(), transfer.friendID(), transfer.sendID(), TOX_FILE_CONTROL_CANCEL, &error);

        Utils::handleFileControlError(error, true); // don't fail on cancel, just log. friend could be off etc.
        updateEventType(transfer, canceledType);
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

    void EventModel::completeTransfer(const Event &transfer, quint64 position)
    {
        quint64 transferID = Utils::transferID(transfer.friendID(), transfer.sendID());
        if ( !fTransferFiles.contains(transferID) ) {
            emit transferError(tr("Unable to find file for transfer"));
            return cancelTransfer(transfer);
        }
        fTransferFiles[transferID]->close();
        delete fTransferFiles[transferID];
        fTransferFiles.remove(transferID);

        EventType eType = transfer.type() == etFileTransferInRunning ? etFileTransferInDone : etFileTransferOutDone;
        QVector<int> roles(2);
        roles[0] = erEventType;
        roles[1] = erFilePosition;
        updateEvent(transfer, eType, position, transfer.filePausers(), roles);
        emit transferComplete(transfer.fileName(), fFriendModel.getListIndexForFriendID(transfer.friendID()), fFriendModel.getFriendByID(transfer.friendID()).name());
    }

    void EventModel::updateEventType(const Event &event, EventType eventType, const QVector<int>& roles)
    {
        updateEvent(event, eventType, event.filePosition(), event.filePausers(), roles);
    }

    void EventModel::updateEvent(const Event &event, EventType eventType, quint64 filePosition, int filePausers, const QVector<int>& roles)
    {
        fDBData.updateEvent(event.id(), eventType, filePosition, filePausers);

        int index = -1;
        if ( fFriendID == event.friendID() && (index = indexForEvent(event.id())) >= 0 ) {
            fList[index].setEventType(eventType);
            fList[index].setFilePosition(filePosition);
            fList[index].setFilePausers(filePausers);
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), roles);
        }
    }

    QFile *EventModel::fileForTransfer(const Event &transfer, QIODevice::OpenModeFlag openMode)
    {
        quint64 transferID = Utils::transferID(transfer.friendID(), transfer.sendID());
        QFile* file = fTransferFiles.value(transferID, NULL);
        if ( file == NULL ) { // new
            file = new QFile(transfer.filePath());

            if ( !file->open(openMode) ) {
                emit transferError("Error opening file: " + file->errorString());
                return NULL;
            }
            fTransferFiles[transferID] = file;
        }

        return file;
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
