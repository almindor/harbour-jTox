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

#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <QHash>
#include <QByteArray>
#include <QObject>
#include <QVariant>
#include <QAbstractListModel>
#include "toxcore.h"
#include "toxme.h"
#include "friend.h"
#include "dbdata.h"
#include "avatarprovider.h"

namespace JTOX {

    class FriendModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int unviewedMessages READ getUnviewedMessages NOTIFY unviewedMessagesChanged)
        Q_PROPERTY(QString address READ getAddress NOTIFY activeFriendChanged)
        Q_PROPERTY(QString name READ getName NOTIFY activeFriendChanged)
        Q_PROPERTY(qint64 friend_id READ getFriendID NOTIFY activeFriendChanged)
    public:
        explicit FriendModel(ToxCore& toxcore, DBData& dbData, AvatarProvider* avatarProvider);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QHash<int, QByteArray> roleNames() const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        const Friend& getFriendByID(quint32 friend_id) const;
        int getListIndexForFriendID(quint32 friend_id) const;
        quint32 getFriendIDByIndex(int index) const;
        void unviewedMessageReceived(quint32 friend_id);
        void messagesViewed(quint32 friend_id);
        const QString getAddress() const;
        const QString getName() const;
        qint64 getFriendID() const;
        Q_INVOKABLE void setOfflineName(const QString& name);
        Q_INVOKABLE void addFriend(const QString& address, const QString& message);
        Q_INVOKABLE void addFriendNoRequest(const QString& publicKey, const QString& name);
        Q_INVOKABLE void removeFriend(quint32 friendID);
        Q_INVOKABLE void setActiveFriendID(quint32 friendID);
    signals:
        void friendUpdated(quint32 friend_id) const;
        void friendAdded() const;
        void friendAddError(const QString& error) const;
        void unviewedMessagesChanged(int count) const;
        void activeFriendChanged(int friendIndex) const;
        void friendWentOnline(int friendID) const;
    public slots:
        void onProfileAvatarChanged(const QByteArray& hash, const QByteArray& data);
    private slots:
        void refresh();
        void onFriendStatusChanged(quint32 friend_id, int status);
        void onFriendConStatusChanged(quint32 friend_id, int status);
        void onFriendStatusMsgChanged(quint32 friend_id, const QString& statusMessage);
        void onFriendNameChanged(quint32 friend_id, const QString& name);
        void onFriendTypingChanged(quint32 friend_id, bool typing);
    private:
        ToxCore& fToxCore;
        DBData& fDBData;
        AvatarProvider* fAvatarProvider; // pointer because freed by QT5
        FriendList fList;
        QString fFriendMessage;
        int fUnviewedMessages;
        int fActiveFriendIndex;

        bool handleFriendRequestError(TOX_ERR_FRIEND_ADD error, QString& errorOut) const;
        bool handleFriendDeleteError(TOX_ERR_FRIEND_DELETE error) const;
        void checkUnviewedTotals();
        int getUnviewedMessages() const;
        void onFriendWentOnline(int index);
    };

}

#endif // FRIENDMODEL_H
