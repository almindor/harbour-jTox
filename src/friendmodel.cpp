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

#include "friendmodel.h"
#include "utils.h"

namespace JTOX {

    FriendModel::FriendModel(ToxCore& toxcore, DBData& dbData, AvatarProvider* avatarProvider) : QAbstractListModel(0),
        fToxCore(toxcore), fDBData(dbData), fAvatarProvider(avatarProvider),
        fList(), fFriendMessage(), fUnviewedMessages(0)
    {        
        connect(&toxcore, &ToxCore::clientReset, this, &FriendModel::refresh);
        connect(&toxcore, &ToxCore::friendStatusChanged, this, &FriendModel::onFriendStatusChanged);
        connect(&toxcore, &ToxCore::friendConStatusChanged, this, &FriendModel::onFriendConStatusChanged);
        connect(&toxcore, &ToxCore::friendStatusMsgChanged, this, &FriendModel::onFriendStatusMsgChanged);
        connect(&toxcore, &ToxCore::friendNameChanged, this, &FriendModel::onFriendNameChanged);
        connect(&toxcore, &ToxCore::friendTypingChanged, this, &FriendModel::onFriendTypingChanged);
    }

    int FriendModel::rowCount(const QModelIndex &parent) const {
        Q_UNUSED(parent);

        return fList.size();
    }

    QHash<int, QByteArray> FriendModel::roleNames() const {
        QHash<int, QByteArray> result;
        result[frName] = "name";
        result[frStatus] = "status";
        result[frStatusMessage] = "status_message";
        result[frTyping] = "typing";
        result[frFriendID] = "friend_id";
        result[frPublicKey] = "public_key";
        result[frUnviewed] = "unviewed";

        return result;
    }

    QVariant FriendModel::data(const QModelIndex &index, int role) const {
        if ( index.row() < 0 || index.row() >= fList.size() ) {
            Utils::bail("Friend data out of bounds");
        }

        return fList.at(index.row()).value(role);
    }

    const Friend& FriendModel::getFriendByID(quint32 friend_id) const {
        int index = getListIndexForFriendID(friend_id);
        if ( index < 0 || index >= fList.size() ) {
            Utils::bail("FriendID not found in model list");
        }

        return fList.at(index);
    }

    void FriendModel::addFriend(const QString& address, const QString& message) {
        if ( !fToxCore.getInitialized() ) {
            Utils::bail("Friend add called when toxcore not initialized!");
        }

        TOX_ERR_FRIEND_ADD error;
        const QByteArray rawMsg = message.toUtf8();
        const QByteArray rawAddress = QByteArray::fromHex(address.toUtf8());

        quint32 friendID = tox_friend_add(fToxCore.tox(), (uint8_t*) rawAddress.data(), (uint8_t*) rawMsg.data(), rawMsg.size(), &error);
        QString errorStr;
        if ( handleFriendRequestError(error, errorStr) ) {
            beginInsertRows(QModelIndex(), fList.size(), fList.size());
            fList.append(Friend(fToxCore, friendID));
            endInsertRows();

            fToxCore.save();
            emit friendAdded();
        } else {
            emit friendAddError(errorStr);
        }
    }

    void FriendModel::addFriendNoRequest(const QString& publicKey, const QString& name) {
        if ( !fToxCore.getInitialized() ) {
            Utils::bail("Friend add (nor) called when toxcore not initialized!");
        }

        TOX_ERR_FRIEND_ADD error;
        const QByteArray rawKey = QByteArray::fromHex(publicKey.toUtf8());

        quint32 friendID = tox_friend_add_norequest(fToxCore.tox(), (uint8_t*) rawKey.data(), &error);
        QString errorStr;
        if ( handleFriendRequestError(error, errorStr) ) {
            beginInsertRows(QModelIndex(), fList.size(), fList.size());
            fList.append(Friend(fToxCore, friendID));
            fList.last().setOfflineName(name);
            fDBData.setFriendOfflineName(fList.last().address(), fList.last().friendID(), name);
            endInsertRows();
            fToxCore.save();
        } else {
            emit friendAddError(errorStr);
        }
    }

    void FriendModel::removeFriend(quint32 friendID)
    {
        if ( !fToxCore.getInitialized() ) {
            Utils::bail("Friend remove called when toxcore not initialized!");
        }

        int index = getListIndexForFriendID(friendID);
        if ( index < 0 || index >= fList.size() ) {
            Utils::bail("FriendID not found in model list");
        }

        TOX_ERR_FRIEND_DELETE error;
        tox_friend_delete(fToxCore.tox(), friendID, &error);

        if ( !handleFriendDeleteError(error) ) {
            return;
        }

        beginRemoveRows(QModelIndex(), index, index);
        fList.removeAt(index);
        fDBData.wipe(friendID);
        endRemoveRows();
    }

    void FriendModel::setActiveFriendID(quint32 friendID)
    {
        fActiveFriendIndex = getListIndexForFriendID(friendID);
        emit activeFriendChanged(fActiveFriendIndex);
    }

    void FriendModel::refresh() {
        if ( !fToxCore.getInitialized() ) {
            Utils::bail("Refreshing friend list with uninitialized tox instance");
        }
        beginResetModel();

        size_t i;
        fList.clear();
        size_t size = tox_self_get_friend_list_size(fToxCore.tox());
        uint32_t raw_list[size];
        tox_self_get_friend_list(fToxCore.tox(), raw_list);

        for ( i = 0; i < size; i++ ) {
            fList.append(Friend(fToxCore, raw_list[i]));
            if ( fDBData.getUnviewedEventCount(raw_list[i]) > 0 ) {
                fList[i].setUnviewed();
            }

            fList[i].setOfflineName(fDBData.getFriendOfflineName(fList.at(i).address()));
        }

        fUnviewedMessages = fDBData.getUnviewedEventCount(-1);
        endResetModel();

        if ( fUnviewedMessages > 0 ) {
            emit unviewedMessagesChanged(fUnviewedMessages);
        }
    }

    void FriendModel::onFriendStatusChanged(quint32 friend_id, int status)
    {
        int index = getListIndexForFriendID(friend_id);
        fList[index].setStatus(status);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        emit friendUpdated(friend_id);
    }

    void FriendModel::onFriendConStatusChanged(quint32 friend_id, int status)
    {
        int index = getListIndexForFriendID(friend_id);
        int oldStatus = fList.at(index).status();
        fList[index].setConStatus(status);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        emit friendUpdated(friend_id);
        if  ( oldStatus == 0 && fList.at(index).status() > 0 ) {
            onFriendWentOnline(index);
            emit friendWentOnline(friend_id); // we attempt sending all offline messages in eventmodel in this case
        }
    }

    void FriendModel::onFriendStatusMsgChanged(quint32 friend_id, const QString& statusMessage)
    {
        int index = getListIndexForFriendID(friend_id);
        fList[index].setStatusMessage(statusMessage);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        emit friendUpdated(friend_id);
    }

    void FriendModel::onFriendNameChanged(quint32 friend_id, const QString& name)
    {
        int index = getListIndexForFriendID(friend_id);
        fList[index].setName(name);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        emit friendUpdated(friend_id);
    }

    void FriendModel::onFriendTypingChanged(quint32 friend_id, bool typing)
    {
        int index = getListIndexForFriendID(friend_id);
        fList[index].setTyping(typing);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        emit friendUpdated(friend_id);
    }

    void FriendModel::onProfileAvatarChanged(const QByteArray& hash, const QByteArray& data)
    {
        for ( int i = 0; i < fList.size(); i++ ) {
            if ( !fList.at(i).isOnline() || fList.at(i).avatarHash() == hash ) {
                continue; // skip offliners or friends who have this already
            }

            fToxCore.sendAvatar(fList.at(i).friendID(), hash, data);
            fList[i].setAvatarHash(hash);
        }
    }

    void FriendModel::onFriendAvatarChanged(quint32 friend_id)
    {
        int index = getListIndexForFriendID(friend_id);
        emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        emit friendUpdated(friend_id);
    }

    int FriendModel::getListIndexForFriendID(quint32 friend_id) const {
        int i = 0;
        foreach ( const Friend& fr, fList ) {
            if ( fr.friendID() == friend_id ) {
                return i;
            }
            i++;
        }

        Utils::bail("Invalid friend ID received on getListIndexForFriendID: " + QString::number(friend_id, 10));
        return -1;
    }

    quint32 FriendModel::getFriendIDByIndex(int index) const
    {
        if ( index < 0 || index >= fList.size() ) {
            Utils::bail("Friend index out of bounds");
        }

        return fList.at(index).friendID();
    }

    void FriendModel::unviewedMessageReceived(quint32 friend_id)
    {
        int index = getListIndexForFriendID(friend_id);
        if ( !fList.at(index).unviewed() ) {
            fList[index].setUnviewed();
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        }

        checkUnviewedTotals();
    }

    void FriendModel::messagesViewed(quint32 friend_id)
    {
        int index = getListIndexForFriendID(friend_id);
        if ( fList.at(index).unviewed() ) {
            fList[index].setViewed();
            emit dataChanged(createIndex(index, 0), createIndex(index, 0), QVector<int>());
        }

        checkUnviewedTotals();
    }

    const QString FriendModel::getAddress() const
    {
        if (fActiveFriendIndex < 0 || fActiveFriendIndex >= fList.size()) {
            return QString();
        }

        return fList.at(fActiveFriendIndex).address();
    }

    const QString FriendModel::getName() const
    {
        if (fActiveFriendIndex < 0 || fActiveFriendIndex >= fList.size()) {
            return QString();
        }

        return fList.at(fActiveFriendIndex).name();
    }

    void FriendModel::setOfflineName(const QString& name)
    {
        if (fActiveFriendIndex < 0 || fActiveFriendIndex >= fList.size()) {
            Utils::bail("Active friend index out of bounds: " + fActiveFriendIndex);
        }

        fList[fActiveFriendIndex].setOfflineName(name);
        const Friend& fr = fList.at(fActiveFriendIndex);
        fDBData.setFriendOfflineName(fr.address(), fr.friendID(), name);

        emit dataChanged(createIndex(fActiveFriendIndex, 0), createIndex(fActiveFriendIndex, 0), QVector<int>());
    }

    bool FriendModel::handleFriendRequestError(TOX_ERR_FRIEND_ADD error, QString& errorOut) const
    {
        errorOut.clear();
        switch ( error ) {
            case TOX_ERR_FRIEND_ADD_ALREADY_SENT: errorOut = "Friend request already sent"; break;
            case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM: errorOut = "Bad checksum on friend request"; break;
            case TOX_ERR_FRIEND_ADD_MALLOC: Utils::bail("Malloc error on friend request"); break; // fatal
            case TOX_ERR_FRIEND_ADD_NO_MESSAGE: errorOut = "No message specified on friend request"; break;
            case TOX_ERR_FRIEND_ADD_NULL: Utils::bail("Null error on friend request"); break; // fatal
            case TOX_ERR_FRIEND_ADD_OWN_KEY: errorOut = "Friend request attempted with own key"; break;
            case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM: errorOut = "Friend request failed, nospam different"; break;
            case TOX_ERR_FRIEND_ADD_TOO_LONG: errorOut = "Friend request message too long"; break;
            case TOX_ERR_FRIEND_ADD_OK: return true;
        }

        if ( !errorOut.isEmpty() ) {
            return false;
        }

        Utils::bail("Unknown error");
        return false;
    }

    bool FriendModel::handleFriendDeleteError(TOX_ERR_FRIEND_DELETE error) const
    {
        switch ( error ) {
            case TOX_ERR_FRIEND_DELETE_FRIEND_NOT_FOUND: Utils::bail("Friend not found");
            case TOX_ERR_FRIEND_DELETE_OK: return true;
        }

        qDebug() << "Not supposed to be here\n";
        return false;
    }

    void FriendModel::checkUnviewedTotals()
    {
        fUnviewedMessages = fDBData.getUnviewedEventCount(-1);
        emit unviewedMessagesChanged(fUnviewedMessages);
    }

    int FriendModel::getUnviewedMessages() const
    {
        return fUnviewedMessages;
    }

    void FriendModel::onFriendWentOnline(int index)
    {
        const QByteArray data = fAvatarProvider->getProfileAvatarData();

        if ( data.isEmpty() || index < 0 || index >= fList.size() ) {
            return;
        }

        const QByteArray hash = fToxCore.hash(data);

        if ( fList.at(index).avatarHash() != hash ) {
            fToxCore.sendAvatar(fList.at(index).friendID(), hash, data);
            fList[index].setAvatarHash(hash);
        }
    }

}
