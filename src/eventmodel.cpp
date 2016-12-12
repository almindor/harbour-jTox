#include "eventmodel.h"
#include "utils.h"
#include "friend.h"
#include <QDateTime>
#include <QDebug>

namespace JTOX {

    EventModel::EventModel(ToxCore& toxCore, FriendModel& friendModel, DBData& dbData) : QAbstractListModel(0),
                    fToxCore(toxCore), fFriendModel(friendModel), fDBData(dbData),
                    fList(), fTimer(), fFriendID(-1), fTyping(false)
    {
        connect(&toxCore, &ToxCore::messageDelivered, this, &EventModel::onMessageDelivered);
        connect(&toxCore, &ToxCore::messageReceived, this, &EventModel::onMessageReceived);
        connect(&friendModel, &FriendModel::friendUpdated, this, &EventModel::onFriendUpdated);
        connect(&fTimer, &QTimer::timeout, this, &EventModel::onMessagesViewed);

        fTimer.setInterval(2000); // 2 sec after viewing we consider msgs read TODO: combine with actually viewed msgs from QML
        fTimer.setSingleShot(true);
    }

    EventModel::~EventModel() {
        fDB.close();
    }

    QHash<int, QByteArray> EventModel::roleNames() const {
        QHash<int, QByteArray> result;
        result[erID] = "id";
        result[erEventType] = "event_type";
        result[erCreated] = "created_at";
        result[erMessage] = "message";

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

    qint64 EventModel::setFriendIndex(int friendIndex)
    {
        setFriend(fFriendModel.getFriendIDByIndex(friendIndex));
        return fFriendID;
    }

    void EventModel::setFriend(qint64 friendID) {
        if ( fFriendID == friendID ) {
            return;
        }

        fFriendID = friendID;
        //qDebug() << "Set friend_id: " << friendID << "\n";

        if ( fFriendID < 0 ) {
            fTimer.stop(); // make sure we don't try to mark someone else's messages by accident
            return;
        }

        beginResetModel();
        fDBData.getEvents(fList, fFriendID);
        endResetModel();

        emit friendUpdated();
        fTimer.start();
    }

    void EventModel::sendMessage(const QString& message) {
        if ( !fToxCore.getInitialized() ) {
            Utils::bail("Send called when toxcore not initialized!");
        }

        int count = rowCount();
        const QByteArray rawMsg = message.toUtf8();
        TOX_ERR_FRIEND_SEND_MESSAGE error;
        quint32 sendID = tox_friend_send_message(fToxCore.tox(), fFriendID, TOX_MESSAGE_TYPE_NORMAL, (uint8_t*) rawMsg.data(), rawMsg.size(), &error);

        if ( !handleSendMessageError(error) ) {
            return;
        }

        QDateTime createdAt;
        int id = fDBData.insertEvent(sendID, fFriendID, etMessageOutPending, message, createdAt);

        beginInsertRows(QModelIndex(), count, count);
        fList.append(Event(id, fFriendID, createdAt, etMessageOutPending, message, sendID));
        endInsertRows();
    }

    void EventModel::stopTyping(qint64 friendID)
    {
        setTyping(false, friendID);
    }

    void EventModel::onMessageDelivered(quint32 friendID, quint32 sendID) {
        fDBData.deliverEvent(sendID, friendID);

        // if we're "open" on the given friend, make sure to update the UI
        if ( fFriendID == friendID ) {
            int row = 0;
            foreach ( const Event event, fList ) {
                if ( event.sendID() >= 0 && event.sendID() == sendID ) {
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

    bool EventModel::handleSendMessageError(TOX_ERR_FRIEND_SEND_MESSAGE error) const
    {
        switch ( error ) {
            case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY: Utils::bail("Cannot send empty message");
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED: Utils::bail("Cannot send message, friend not online");
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND: Utils::bail("Cannot send message, friend not found");
            case TOX_ERR_FRIEND_SEND_MESSAGE_NULL: Utils::bail("Cannot send null message");
            case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ: Utils::bail("Cannot send message, sendq error");
            case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG: Utils::bail("Cannot send message it is too long");
            case TOX_ERR_FRIEND_SEND_MESSAGE_OK: return true;
        }

        Utils::bail("Unknown error");
        return false;
    }

    int EventModel::getFriendStatus() const {
        // TODO: optimize, use index
        return fFriendModel.getFriendByID(fFriendID).status();
    }

    bool EventModel::getFriendTyping() const {
        return fFriendModel.getFriendByID(fFriendID).typing();
    }

    const QString EventModel::getFriendName() const {
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
        if ( typing == fTyping || friendID < 0 ) {
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

}
