#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariant>
#include <QTimer>
#include <tox/tox.h>
#include "toxcore.h"
#include "friendmodel.h"
#include "event.h"
#include "encryptsave.h"
#include "dbdata.h"

namespace JTOX {

    class EventModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int friendID READ getFriendID NOTIFY friendUpdated)
        Q_PROPERTY(QString friendName READ getFriendName NOTIFY friendUpdated)
        Q_PROPERTY(int friendStatus READ getFriendStatus NOTIFY friendUpdated)
        Q_PROPERTY(bool friendTyping READ getFriendTyping NOTIFY friendUpdated)
        Q_PROPERTY(bool typing READ getTyping WRITE setTyping NOTIFY typingChanged)
    public:
        EventModel(ToxCore& toxCore, FriendModel& friendModel, DBData& dbData);
        virtual ~EventModel();
        QHash<int, QByteArray> roleNames() const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        int getFriendID() const;

        Q_INVOKABLE qint64 setFriendIndex(int friendIndex);
        Q_INVOKABLE void setFriend(qint64 friendID);
        Q_INVOKABLE void sendMessage(const QString& message);
        Q_INVOKABLE void stopTyping(qint64 friendID);
    signals:
        void friendUpdated() const;
        void typingChanged(bool typing) const;
        void messageReceived(int friendIndex, const QString& friendName) const;
    public slots:
        void onMessageDelivered(quint32 friendID, quint32 sendID);
        void onMessageReceived(quint32 friend_id, TOX_MESSAGE_TYPE type, const QString& message);
        void onFriendUpdated(quint32 friend_id);
    private:
        ToxCore& fToxCore;
        FriendModel& fFriendModel;
        DBData fDBData;
        EventList fList;
        QTimer fTimer;
        QSqlDatabase fDB;
        QSqlQuery fSelectQuery;
        QSqlQuery fInsertQuery;
        QSqlQuery fDeliveredUpdateQuery;
        qint64 fFriendID;
        bool fTyping;

        bool handleSendMessageError(TOX_ERR_FRIEND_SEND_MESSAGE error) const;
        int getFriendStatus() const;
        bool getFriendTyping() const;
        const QString getFriendName() const;
        bool getTyping() const;
        void setTyping(bool typing);
        void setTyping(qint64 friendID, bool typing);
    private slots:
        void onMessagesViewed();
    };

}

#endif // EVENTMODEL_H
