#ifndef DBDATA_H
#define DBDATA_H

#include "event.h"
#include "friendrequest.h"
#include "encryptsave.h"
#include <QString>
#include <QSqlDatabase>
#include <QDateTime>
#include <QSqlQuery>

namespace JTOX {

    class DBData
    {
    public:
        DBData(EncryptSave& encryptSave);
        bool getEvent(int event_id, Event& result);
        bool getEvent(quint32 friend_id, quint32 send_id, EventType event_type, Event& result);
        void getEvents(EventList& list, quint32 friendID, int eventType = -1);
        bool getCall(quint32 friend_id, Event& result);
        void getTransfers(EventList& list);
        int getUnviewedEventCount(qint64 friendID);
        int insertEvent(Event& event);
        void updateEventType(int id, EventType eventType);
        void updateEvent(int id, EventType eventType, quint64 filePosition, int filePausers);
        void updateEventSent(int id, EventType eventType, qint64 sendID);
        void deliverEvent(quint32 sendID, quint32 friendID);
        void deleteEvent(int id);
        void insertRequest(FriendRequest& request);
        void updateRequest(const FriendRequest& request);
        void deleteRequest(const FriendRequest& request);
        bool getAvatar(qint64 friend_id, QByteArray& result); // -1 for "me"
        bool checkAvatar(qint64 friend_id, const QByteArray& hash);
        void clearAvatar(qint64 friend_id);
        void setAvatar(qint64 friend_id, const QByteArray& hash, const QByteArray& data);
        void getRequests(RequestList& list);
        void setFriendOfflineName(const QString& address, quint32 friendID, const QString& name);
        const QString getFriendOfflineName(const QString& address);
        void wipe(qint64 friendID);
        void wipeLogs();
    private:
        EncryptSave& fEncryptSave;
        QSqlDatabase fDB;
        QSqlQuery fEventSelectOneQuery;
        QSqlQuery fCallSelectOneQuery;
        QSqlQuery fEventSelectQuery;
        QSqlQuery fLastEventSelectQuery;
        QSqlQuery fTransfersSelectQuery;
        QSqlQuery fEventUnviewedCountQuery;
        QSqlQuery fEventInsertQuery;
        QSqlQuery fEventUpdateQuery;
        QSqlQuery fEventUpdateSentQuery;
        QSqlQuery fEventDeliveredQuery;
        QSqlQuery fEventDeleteQuery;
        QSqlQuery fRequestSelectQuery;
        QSqlQuery fRequestInsertQuery;
        QSqlQuery fRequestUpdateQuery;
        QSqlQuery fRequestDeleteQuery;
        QSqlQuery fLastRequestSelectQuery;
        QSqlQuery fFriendOfflineNameSelectQuery;
        QSqlQuery fFriendOfflineNameUpdateQuery;
        QSqlQuery fWipeEventsQuery;
        QSqlQuery fWipeRequestsQuery;
        QSqlQuery fWipeFriendsQuery;
        QSqlQuery fGetAvatarQuery;
        QSqlQuery fCheckAvatarQuery;
        QSqlQuery fSetAvatarQuery;
        QSqlQuery fClearAvatarQuery;
        void createTables();
        void upgradeToV1(); // v0 to v1 upgrade
        void upgradeToV2(); // v1 to v2 upgrade
        void prepareQueries();
        const Event parseEvent(const QSqlQuery& query) const;
        const QSqlQuery prepareQuery(const QString& sql);
        int userVersion() const;
        void setUserVersion(int version);
    };

}

#endif // DBDATA_H
