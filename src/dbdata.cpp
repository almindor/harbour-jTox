#include "dbdata.h"
#include "utils.h"
#include <QDir>
#include <QStandardPaths>
#include <QSqlError>
#include <QDebug>

namespace JTOX {

    DBData::DBData(EncryptSave& encryptSave) :
        fEncryptSave(encryptSave),
        fDB(QSqlDatabase::addDatabase("QSQLITE"))
    {
        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        if ( !dir.exists() ) {
            dir.mkpath(dir.absolutePath());
        }
        fDB.setDatabaseName(dir.absoluteFilePath("jtox.sqlite"));
        fDB.open();
        if ( !fDB.open() ) {
            Utils::bail( fDB.lastError().text() );
        }

        switch ( userVersion() ) {
            case 0: createTables(); upgradeToV1(); // empty or unversioned (1.2.0-)
            case 1: upgradeToV2();
        }
        prepareQueries();
    }

    bool DBData::getEvent(int event_id, Event& result)
    {
        fEventSelectOneQuery.bindValue(":id", event_id);
        fEventSelectOneQuery.bindValue(":friend_id", -1);
        fEventSelectOneQuery.bindValue(":send_id", -1);
        fEventSelectOneQuery.bindValue(":event_type", -1);

        if ( !fEventSelectOneQuery.exec() ) {
            Utils::bail("Error on event select query exec: " + fEventSelectOneQuery.lastError().text());
        }

        if ( !fEventSelectOneQuery.next() ) {
            return false;
        }

        result = parseEvent(fEventSelectOneQuery);
        return true;
    }

    bool DBData::getEvent(quint32 friend_id, quint32 send_id, EventType event_type, Event& result)
    {
        fEventSelectOneQuery.bindValue(":id", -1);
        fEventSelectOneQuery.bindValue(":friend_id", friend_id);
        fEventSelectOneQuery.bindValue(":send_id", send_id);
        fEventSelectOneQuery.bindValue(":event_type", event_type);

        if ( !fEventSelectOneQuery.exec() ) {
            Utils::bail("Error on event select query exec: " + fEventSelectOneQuery.lastError().text());
        }

        if ( !fEventSelectOneQuery.next() ) {
            return false;
        }

        result = parseEvent(fEventSelectOneQuery);

        return true;
    }

    void DBData::getEvents(EventList& list, quint32 friendID, int eventType)
    {
        fEventSelectQuery.bindValue(":friend_id", friendID);
        fEventSelectQuery.bindValue(":event_type", eventType);

        if ( !fEventSelectQuery.exec() ) {
            Utils::bail("Error on event select query exec: " + fEventSelectQuery.lastError().text());
        }

        list.clear();
        while ( fEventSelectQuery.next() ) {
            list.push_front(parseEvent(fEventSelectQuery));
        }
    }

    void DBData::getTransfers(EventList &list)
    {
        if ( !fTransfersSelectQuery.exec() ) {
            Utils::bail("Error on transfers select query exec: " + fTransfersSelectQuery.lastError().text());
        }


        list.clear();
        while ( fTransfersSelectQuery.next() ) {
            list.append(parseEvent(fTransfersSelectQuery));
        }
    }

    int DBData::getUnviewedEventCount(qint64 friendID)
    {
        // can't use same placeholder in query so we must use 2 placeholders with 1 value
        fEventUnviewedCountQuery.bindValue(":friend_id", friendID);
        fEventUnviewedCountQuery.bindValue(":friend_id2", friendID);
        fEventUnviewedCountQuery.bindValue(":event_type", etMessageInUnread);

        if ( !fEventUnviewedCountQuery.exec() ) {
            qDebug() << fEventUnviewedCountQuery.executedQuery() << "\n";
            Utils::bail("Error on unviewed count query exec: " + fEventUnviewedCountQuery.lastError().text());
        }

        if ( !fEventUnviewedCountQuery.first() ) {
            Utils::bail("Unable to fetch count row for unviewed count query");
        }

        bool ok = false;
        int count = fEventUnviewedCountQuery.value(0).toInt(&ok);
        if ( !ok ) {
            Utils::bail("Unable to get event count int");
        }

        return count;
    }

    int DBData::insertEvent(Event& event)
    {
        fEventInsertQuery.bindValue(":send_id", event.sendID() >= 0 ? event.sendID() : QVariant(QVariant::Int));
        fEventInsertQuery.bindValue(":friend_id", event.friendID());
        fEventInsertQuery.bindValue(":event_type", event.type());
        fEventInsertQuery.bindValue(":message", fEncryptSave.encrypt(event.isFile() ? event.fileName() : event.message()));
        fEventInsertQuery.bindValue(":file_path", event.filePath());
        fEventInsertQuery.bindValue(":file_id", event.fileID());
        fEventInsertQuery.bindValue(":file_size", event.fileSize());
        fEventInsertQuery.bindValue(":file_position", event.filePosition());
        fEventInsertQuery.bindValue(":file_pausers", event.filePausers());

        if ( !fEventInsertQuery.exec() ) {
            Utils::bail("Error on insert query execution: " + fEventInsertQuery.lastError().text());
        }

        fLastEventSelectQuery.bindValue(":friend_id", event.friendID());
        if ( !fLastEventSelectQuery.exec() ) {
            Utils::bail("Error on last event query execution: " + fLastEventSelectQuery.lastError().text());
        }
        if ( !fLastEventSelectQuery.first() ) {
            Utils::bail("Error on last event query fetch: " + fLastEventSelectQuery.lastError().text());
        }

        bool ok = false;
        const QVariant val = fLastEventSelectQuery.value(0);
        int id = val.toInt(&ok);
        if ( !ok ) {
            Utils::bail("Error on last event query int cast: " + val.toString());
        }
        event.setID(id);
        event.setCreatedAt(fLastEventSelectQuery.value(1).toDateTime());
        return id;
    }

    void DBData::updateEventType(int id, EventType eventType)
    {
        return updateEvent(id, eventType, 0, 0);
    }

    void DBData::deliverEvent(quint32 sendID, quint32 friendID)
    {
        fEventDeliveredQuery.bindValue(":friend_id", friendID);
        fEventDeliveredQuery.bindValue(":send_id", sendID);

        if ( !fEventDeliveredQuery.exec() ) {
            Utils::bail("Unable to update event to delivered state: " + fEventDeliveredQuery.lastError().text());
        }
    }

    void DBData::deleteEvent(int id)
    {
        fEventDeleteQuery.bindValue(":id", id);

        if ( !fEventDeleteQuery.exec() ) {
            Utils::bail("Unable to delete event: " + fEventDeleteQuery.lastError().text());
        }
    }

    void DBData::insertRequest(FriendRequest& request)
    {
        fRequestInsertQuery.bindValue(":address", request.getAddress());
        fRequestInsertQuery.bindValue(":message", request.getMessage());
        fRequestInsertQuery.bindValue(":name", request.getName());

        if ( !fRequestInsertQuery.exec() ) {
            Utils::bail("Unable to insert request: " + fRequestInsertQuery.lastError().text());
        }

        if ( !fLastRequestSelectQuery.exec() ) {
            Utils::bail("Error on last request query execution: " + fLastRequestSelectQuery.lastError().text());
        }

        if ( !fLastRequestSelectQuery.first() ) {
            Utils::bail("Error on last request query fetch: " + fLastRequestSelectQuery.lastError().text());
        }

        bool ok = false;
        const QVariant val = fLastRequestSelectQuery.value(0);
        int id = val.toInt(&ok);
        if ( !ok ) {
            Utils::bail("Error on last request query int cast: " + val.toString());
        }

        request.setID(id);
    }

    void DBData::updateRequest(const FriendRequest& request)
    {
        fRequestUpdateQuery.bindValue(":id", request.getID());
        fRequestUpdateQuery.bindValue(":name", request.getName());

        if ( !fRequestUpdateQuery.exec() ) {
            Utils::bail("Unable to update request: " + fRequestUpdateQuery.lastError().text());
        }
    }

    void DBData::deleteRequest(const FriendRequest& request)
    {
        fRequestDeleteQuery.bindValue(":id", request.getID());
        fRequestDeleteQuery.bindValue(":id2", request.getID()); // double bind trick

        if ( !fRequestDeleteQuery.exec() ) {
            Utils::bail("Unable to delete request: " + fRequestDeleteQuery.lastError().text());
        }
    }

    bool DBData::getAvatar(qint64 friend_id, QByteArray &result)
    {
        fGetAvatarQuery.bindValue(":friend_id", friend_id);

        if ( !fGetAvatarQuery.exec() ) {
            Utils::bail("Unable to get avatar data: " + fGetAvatarQuery.lastError().text());
        }

        if ( fGetAvatarQuery.next() && !fGetAvatarQuery.value(0).isNull() ) {
            result = fGetAvatarQuery.value(0).toByteArray();
            return result.size() > 0;
        }

        return false;
    }

    bool DBData::checkAvatar(qint64 friend_id, const QByteArray& hash)
    {
        fCheckAvatarQuery.bindValue(":friend_id", friend_id);
        fCheckAvatarQuery.bindValue(":hash", hash);

        if ( !fCheckAvatarQuery.exec() ) {
            Utils::bail("Unable to check avatar hash: " + fCheckAvatarQuery.lastError().text());
        }

        if ( fCheckAvatarQuery.next() ) {
            bool ok = false;
            int count = fCheckAvatarQuery.value(0).toInt(&ok);

            if ( !ok ) {
                Utils::bail("Unable to parse avatar count integer");
                return false;
            }

            return count > 0;
        }

        return false;
    }

    void DBData::clearAvatar(qint64 friend_id)
    {
        fClearAvatarQuery.bindValue(":friend_id", friend_id); // -1 for "me"

        if ( !fClearAvatarQuery.exec() ) {
            Utils::bail("Unable to clear avatar data: " + fClearAvatarQuery.lastError().text());
        }
    }

    void DBData::setAvatar(qint64 friend_id, const QByteArray &hash, const QByteArray &data)
    {
        if ( data.isEmpty() ) {
            return clearAvatar(friend_id);
        }

        fSetAvatarQuery.bindValue(":friend_id", friend_id); // -1 for "me"
        fSetAvatarQuery.bindValue(":hash", hash);
        fSetAvatarQuery.bindValue(":data", data);

        if ( !fSetAvatarQuery.exec() ) {
            Utils::bail("Unable to save avatar data: " + fSetAvatarQuery.lastError().text());
        }
    }

    void DBData::getRequests(RequestList& list)
    {
        if ( !fRequestSelectQuery.exec() ) {
            Utils::bail("Error on request select query exec: " + fEventSelectQuery.lastError().text());
        }

        while ( fRequestSelectQuery.next() ) {
            const QString address = fRequestSelectQuery.value("address").toString();
            const QString message = fRequestSelectQuery.value("message").toString();
            const QString name = fRequestSelectQuery.value("name").toString();

            list.append(FriendRequest(address, message, name));
        }
    }

    void DBData::setFriendOfflineName(const QString& address, quint32 friendID, const QString& name)
    {
        fFriendOfflineNameUpdateQuery.bindValue(":address", address);
        fFriendOfflineNameUpdateQuery.bindValue(":friend_id", friendID);
        fFriendOfflineNameUpdateQuery.bindValue(":name", name);

        if ( !fFriendOfflineNameUpdateQuery.exec() ) {
            Utils::bail("Unable to update friend: " + fFriendOfflineNameUpdateQuery.lastError().text());
        }
    }

    const QString DBData::getFriendOfflineName(const QString& address)
    {
        fFriendOfflineNameSelectQuery.bindValue(":address", address);

        if ( !fFriendOfflineNameSelectQuery.exec() ) {
            Utils::bail("Unable to select friend: " + fFriendOfflineNameSelectQuery.lastError().text());
        }

        if ( !fFriendOfflineNameSelectQuery.first() ) {
            return QString();
        }

        const QString offlineName = fFriendOfflineNameSelectQuery.value("name").toString();
        return offlineName;
    }

    void DBData::wipe(qint64 friendID)
    {
        fWipeEventsQuery.bindValue(":friend_id", friendID);
        fWipeEventsQuery.bindValue(":friend_id2", friendID);
        fWipeFriendsQuery.bindValue(":friend_id", friendID);
        fWipeFriendsQuery.bindValue(":friend_id2", friendID);

        if ( !fWipeEventsQuery.exec() ) {
            Utils::bail("Unable to wipe events: " + fWipeEventsQuery.lastError().text());
        }

        if ( !fWipeFriendsQuery.exec() ) {
            Utils::bail("Unable to wipe friends: " + fWipeFriendsQuery.lastError().text());
        }

        // only wipe requests if we're doing a full wipe
        if ( friendID < 0 && !fWipeRequestsQuery.exec() ) {
            Utils::bail("Unable to wipe requests: " + fWipeRequestsQuery.lastError().text());
        }
    }

    void DBData::wipeLogs()
    {
        fWipeEventsQuery.bindValue(":friend_id", -1);
        fWipeEventsQuery.bindValue(":friend_id2", -1);

        if ( !fWipeEventsQuery.exec() ) {
            Utils::bail("Unable to wipe events: " + fWipeEventsQuery.lastError().text());
        }
    }

    void DBData::updateEvent(int id, EventType eventType, quint64 filePosition, int filePausers)
    {
        fEventUpdateQuery.bindValue(":id", id);
        fEventUpdateQuery.bindValue(":event_type", eventType);
        fEventUpdateQuery.bindValue(":file_position", filePosition);
        fEventUpdateQuery.bindValue(":file_pausers", filePausers);

        if ( !fEventUpdateQuery.exec() ) {
            Utils::bail("unable to update event: " + fEventUpdateQuery.lastError().text());
        }
    }

    void DBData::updateEventSent(int id, EventType eventType, qint64 sendID)
    {
        fEventUpdateSentQuery.bindValue(":id", id);
        fEventUpdateSentQuery.bindValue(":event_type", eventType);
        fEventUpdateSentQuery.bindValue(":send_id", sendID);

        if ( !fEventUpdateSentQuery.exec() ) {
            Utils::bail("unable to update event: " + fEventUpdateSentQuery.lastError().text());
        }
    }

    void DBData::createTables() {        
        QSqlQuery createTableQuery(fDB);
        // events
        // NOTE: integer in sqlite3 is up to 8 bytes
        if ( !createTableQuery.exec("CREATE TABLE IF NOT EXISTS events("
                                     "id INTEGER PRIMARY KEY,"
                                     "send_id INTEGER,"
                                     "friend_id INTEGER NOT NULL,"
                                     "event_type INTEGER NOT NULL,"
                                     "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                                     "message BLOB)") ) {
            Utils::bail("unable to create events table: " + createTableQuery.lastError().text());
        }
        // requests
        if ( !createTableQuery.exec("CREATE TABLE IF NOT EXISTS requests("
                                     "id INTEGER PRIMARY KEY,"
                                     "address TEXT NOT NULL,"
                                     "message TEXT NOT NULL,"
                                     "name TEXT NOT NULL)") ) {
            Utils::bail("unable to create requests table: " + createTableQuery.lastError().text());
        }
        // friends for offline name storage
        if ( !createTableQuery.exec("CREATE TABLE IF NOT EXISTS friends("
                                     "address TEXT PRIMARY KEY,"
                                     "friend_id INTEGER NOT NULL,"
                                     "name TEXT NOT NULL)") ) {
            Utils::bail("unable to create friends table: " + createTableQuery.lastError().text());
        }
        fDB.commit();
    }

    void DBData::upgradeToV1()
    {
        QSqlQuery query(fDB);
        // unknown if we have v0 to v1 or just init from scratch so these can fail
        if ( !query.exec("ALTER TABLE events ADD COLUMN file_path BLOB") ) Utils::bail("Unable to upgrade DB to v1");
        if ( !query.exec("ALTER TABLE events ADD COLUMN file_id BLOB") ) Utils::bail("Unable to upgrade DB to v1");
        if ( !query.exec("ALTER TABLE events ADD COLUMN file_size INTEGER") ) Utils::bail("Unable to upgrade DB to v1");
        if ( !query.exec("ALTER TABLE events ADD COLUMN file_position INTEGER") ) Utils::bail("Unable to upgrade DB to v1");
        if ( !query.exec("ALTER TABLE events ADD COLUMN file_pausers INTEGER") ) Utils::bail("Unable to upgrade DB to v1"); // how many pausers are there (0-2)

        setUserVersion(1); // commits
    }

    void DBData::upgradeToV2()
    {
        QSqlQuery query(fDB);
        // unknown if we have v0 to v1 or just init from scratch so these can fail
        if ( !query.exec("CREATE TABLE avatars (friend_id INTEGER PRIMARY KEY, hash BLOB NOT NULL, data BLOB NOT NULL)") ) {
            Utils::bail("Unable to create avatars table");
        }

        setUserVersion(2); // commits
    }

    void DBData::prepareQueries()
    {
        fEventSelectOneQuery = prepareQuery("SELECT id, event_type, created_at, message, send_id, friend_id, "
                                            "       file_path, file_id, file_size, file_position, file_pausers "
                                            "FROM events "
                                            "WHERE (id = :id OR :id < 0) "
                                            "AND (friend_id = :friend_id OR :friend_id < 0) "
                                            "AND (send_id = :send_id OR :send_id < 0) "
                                            "AND (event_type = :event_type OR :event_type < 0) ");

        fEventSelectQuery = prepareQuery("SELECT id, event_type, created_at, message, send_id, friend_id, "
                                         "       file_path, file_id, file_size, file_position, file_pausers "
                                         "FROM ("
                                            "SELECT id, event_type, created_at, message, send_id, friend_id, "
                                            "       file_path, file_id, file_size, file_position, file_pausers "
                                            "FROM events "
                                            "WHERE friend_id = :friend_id "
                                            "AND (event_type = :event_type OR :event_type < 0) "
                                            "ORDER BY id DESC "
                                            "LIMIT 100"
                                         ") tmp ORDER BY tmp.id ASC");

        fLastEventSelectQuery = prepareQuery("SELECT id, created_at "
                                             "FROM events "
                                             "WHERE friend_id = :friend_id "
                                             "ORDER BY id DESC "
                                             "LIMIT 1");

        fTransfersSelectQuery = prepareQuery("SELECT id, event_type, created_at, message, send_id, friend_id,"
                                             "       file_path, file_id, file_size, file_position, file_pausers "
                                             "FROM events "
                                             "WHERE event_type IN (10, 11, 12, 13, 16, 17) "
                                             "ORDER BY id DESC "
                                             "LIMIT 100");

        fEventUnviewedCountQuery = prepareQuery("SELECT count(*) FROM events WHERE event_type = :event_type AND (friend_id = :friend_id OR :friend_id2 < 0)");
        fEventInsertQuery = prepareQuery("INSERT INTO events(send_id, friend_id, event_type, message, file_path, file_id, file_size, file_position, file_pausers) VALUES(:send_id, :friend_id, :event_type, :message, :file_path, :file_id, :file_size, :file_position, :file_pausers)");
        fEventUpdateQuery = prepareQuery("UPDATE events SET event_type = :event_type, file_position = :file_position, file_pausers = :file_pausers WHERE id = :id");
        fEventUpdateSentQuery = prepareQuery("UPDATE events SET event_type = :event_type, send_id = :send_id WHERE id = :id");
        fEventDeliveredQuery = prepareQuery("UPDATE events SET event_type = 1 WHERE send_id = :send_id AND friend_id = :friend_id");
        fEventDeleteQuery = prepareQuery("DELETE FROM events WHERE id = :id");

        fRequestSelectQuery = prepareQuery("SELECT id, address, message, name FROM requests");
        fRequestInsertQuery = prepareQuery("INSERT INTO requests(address, message, name) VALUES(:address, :message, :name)");
        fRequestUpdateQuery = prepareQuery("UPDATE requests SET name = :name WHERE id = :id");
        fRequestDeleteQuery = prepareQuery("DELETE FROM requests WHERE id = :id OR :id2 < 0");
        fLastRequestSelectQuery = prepareQuery("SELECT max(id) from requests");

        fFriendOfflineNameSelectQuery = prepareQuery("SELECT name FROM friends WHERE address = :address");
        fFriendOfflineNameUpdateQuery = prepareQuery("INSERT OR REPLACE INTO friends(address, friend_id, name) VALUES(:address, :friend_id, :name)");

        fWipeEventsQuery = prepareQuery("DELETE FROM events WHERE (friend_id = :friend_id OR :friend_id2 < 0)");
        fWipeFriendsQuery = prepareQuery("DELETE FROM friends WHERE (friend_id = :friend_id OR :friend_id2 < 0)");
        fWipeRequestsQuery = prepareQuery("DELETE FROM requests");

        fGetAvatarQuery = prepareQuery("SELECT data FROM avatars WHERE friend_id = :friend_id");
        fCheckAvatarQuery = prepareQuery("SELECT count(*) FROM avatars WHERE friend_id = :friend_id AND hash = :hash");
        fSetAvatarQuery = prepareQuery("INSERT OR REPLACE INTO avatars(friend_id, hash, data) VALUES(:friend_id, :hash, :data)");
        fClearAvatarQuery = prepareQuery("DELETE FROM avatars WHERE friend_id = :friend_id");
    }

    const Event DBData::parseEvent(const QSqlQuery &query) const
    {
        bool ok = false;
        int id = query.value("id").toInt(&ok);
        if ( !ok ) {
            Utils::bail("Error casting event id: " + query.value("id").toString());
        }
        const QString message = fEncryptSave.decrypt(query.value("message").toByteArray());
        int rawEType = query.value("event_type").toInt(&ok);
        if ( !ok ) {
            Utils::bail("Error casting event type: " + query.value("event_type").toString());
        }
        EventType eventType = (EventType) rawEType;
        const QDateTime createdAt = query.value("created_at").toDateTime();
        qint64 sendID = -1;
        if ( !query.value("send_id").isNull() ) {
            sendID = query.value("send_id").toLongLong(&ok);
            if ( !ok ) {
                Utils::bail("Error casting send_id: " + query.value("send_id").toString());
            }
        }
        quint32 friendID = -1;
        if ( !query.value("friend_id").isNull() ) {
            friendID = query.value("friend_id").toUInt(&ok);
            if ( !ok ) {
                Utils::bail("Error casting event friend_id: " + query.value("friend_id").toString());
            }
        }

        QString file_path;
        if ( !query.value("file_path").isNull() ) {
            file_path = query.value("file_path").toString();
        }

        QByteArray file_id;
        if ( !query.value("file_id").isNull() ) {
            file_id = query.value("file_id").toByteArray();
        }

        quint64 file_size = 0;
        if ( !query.value("file_size").isNull() ) {
            file_size = query.value("file_size").toInt(&ok);
            if ( !ok ) {
                Utils::bail("Error casting event file_size: " + query.value("file_size").toString());
            }
        }

        quint64 file_position = 0;
        if ( !query.value("file_position").isNull() ) {
            file_position = query.value("file_position").toInt(&ok);
            if ( !ok ) {
                Utils::bail("Error casting event file_position: " + query.value("file_position").toString());
            }
        }

        int file_pausers = 0;
        if ( !query.value("file_pausers").isNull() ) {
            file_pausers = query.value("file_pausers").toInt(&ok);
            if ( !ok ) {
                Utils::bail("Error casting event file_pausers: " + query.value("file_pausers").toString());
            }
        }

        return Event(id, friendID, createdAt, eventType, message, sendID, file_path, file_id, file_size, file_position, file_pausers);
    }

    const QSqlQuery DBData::prepareQuery(const QString& sql)
    {
        QSqlQuery query(fDB);
        if ( !query.prepare(sql) ) {
            Utils::bail("Unable to prepare query: " + query.lastError().text());
        }

        return query;
    }

    int DBData::userVersion() const
    {
        QSqlQuery query(fDB);
        if ( !query.exec("PRAGMA user_version") || !query.next() ) {
            Utils::bail("unable to query user_version");
            return 0;
        }

        bool ok = false;
        int result = query.value("user_version").toInt(&ok);
        if ( !ok ) {
            Utils::bail("unable to parse user_version");
            return 0;
        }

        return result;
    }

    void DBData::setUserVersion(int version)
    {
        QSqlQuery query(fDB);

        if ( !query.exec("PRAGMA user_version = " + QString::number(version)) ) {
            Utils::bail("unable to set user_version");
            return;
        }

        fDB.commit();
    }

}
