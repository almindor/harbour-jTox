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

        createTables();
        prepareQueries();
    }

    void DBData::getEvents(EventList& list, quint32 friendID)
    {
        fEventSelectQuery.bindValue(":friend_id", friendID);

        if ( !fEventSelectQuery.exec() ) {
            Utils::bail("Error on event select query exec: " + fEventSelectQuery.lastError().text());
        }

        list.clear();
        while ( fEventSelectQuery.next() ) {
            bool ok = false;
            int id = fEventSelectQuery.value("id").toInt(&ok);
            if ( !ok ) {
                Utils::bail("Error casting event id: " + fEventSelectQuery.value("id").toString());
            }
            const QString message = fEncryptSave.decrypt(fEventSelectQuery.value("message").toByteArray());
            int rawEType = fEventSelectQuery.value("event_type").toInt(&ok);
            if ( !ok ) {
                Utils::bail("Error casting event type: " + fEventSelectQuery.value("event_type").toString());
            }
            EventType eventType = (EventType) rawEType;
            const QDateTime createdAt = fEventSelectQuery.value("created_at").toDateTime();
            qint64 sendID = -1;
            if ( !fEventSelectQuery.value("send_id").isNull() ) {
                fEventSelectQuery.value("send_id").toLongLong(&ok);
                if ( !ok ) {
                    Utils::bail("Error casting send_id: " + fEventSelectQuery.value("send_id").toString());
                }
            }

            list.append(Event(id, friendID, createdAt, eventType, message, sendID));
        }
    }

    int DBData::getUnviewedEventCount(qint64 friendID)
    {
        // can't use same placeholder in query so we must use 2 placeholders with 1 value
        fEventUnviewedCountQuery.bindValue(":friend_id", friendID);
        fEventUnviewedCountQuery.bindValue(":friend_id2", friendID);

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

    int DBData::insertEvent(qint64 sendID, quint32 friendID, EventType eventType, const QString& message, QDateTime& createdAt)
    {
        fEventInsertQuery.bindValue(":send_id", sendID >= 0 ? sendID : QVariant(QVariant::Int));
        fEventInsertQuery.bindValue(":friend_id", friendID);
        fEventInsertQuery.bindValue(":event_type", eventType);
        fEventInsertQuery.bindValue(":message", fEncryptSave.encrypt(message));

        if ( !fEventInsertQuery.exec() ) {
            Utils::bail("Error on insert query execution: " + fEventInsertQuery.lastError().text());
        }

        fLastEventSelectQuery.bindValue(":friend_id", friendID);
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
        createdAt = fLastEventSelectQuery.value(1).toDateTime();
        return id;
    }

    void DBData::deliverEvent(quint32 sendID, quint32 friendID)
    {
        fEventDeliveredQuery.bindValue(":friend_id", friendID);
        fEventDeliveredQuery.bindValue(":send_id", sendID);

        if ( !fEventDeliveredQuery.exec() ) {
            Utils::bail("Unable to update event to delivered state: " + fEventDeliveredQuery.lastError().text());
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

    void DBData::updateEventType(int id, EventType eventType)
    {
        fEventUpdateQuery.bindValue(":id", id);
        fEventUpdateQuery.bindValue(":event_type", eventType);

        if ( !fEventUpdateQuery.exec() ) {
            Utils::bail("unable to update event: " + fEventUpdateQuery.lastError().text());
        }
    }

    void DBData::createTables() {
        QSqlQuery createTableQuery(fDB);
        // events
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

    void DBData::prepareQueries()
    {
        fEventSelectQuery = prepareQuery("SELECT id, event_type, created_at, message, send_id "
                                         "FROM ("
                                            "SELECT id, event_type, created_at, message, send_id "
                                            "FROM events "
                                            "WHERE friend_id = :friend_id "
                                            "ORDER BY id DESC "
                                            "LIMIT 100"
                                         ") tmp ORDER BY tmp.id ASC");

        fLastEventSelectQuery = prepareQuery("SELECT id, created_at "
                                             "FROM events "
                                             "WHERE friend_id = :friend_id "
                                             "ORDER BY id DESC "
                                             "LIMIT 1");

        fEventUnviewedCountQuery = prepareQuery("SELECT count(*) FROM events WHERE event_type = 2 AND (friend_id = :friend_id OR :friend_id2 < 0)");
        fEventInsertQuery = prepareQuery("INSERT INTO events(send_id, friend_id, event_type, message) VALUES(:send_id, :friend_id, :event_type, :message)");
        fEventUpdateQuery = prepareQuery("UPDATE events SET event_type = :event_type WHERE id = :id");
        fEventDeliveredQuery = prepareQuery("UPDATE events SET event_type = 1 WHERE send_id = :send_id AND friend_id = :friend_id");

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
    }

    const QSqlQuery DBData::prepareQuery(const QString& sql)
    {
        QSqlQuery query(fDB);
        if ( !query.prepare(sql) ) {
            Utils::bail("Unable to prepare query: " + query.lastError().text());
        }

        return query;
    }

}
