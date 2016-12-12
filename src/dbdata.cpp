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

    void DBData::wipe(qint64 friendID)
    {
        fWipeQuery.bindValue(":friend_id", friendID);
        fWipeQuery.bindValue(":friend_id2", friendID);

        if ( !fWipeQuery.exec() ) {
            Utils::bail("Unable to wipe events: " + fWipeQuery.lastError().text());
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
        if ( !createTableQuery.exec("CREATE TABLE IF NOT EXISTS events("
                                     "id INTEGER PRIMARY KEY,"
                                     "send_id INTEGER,"
                                     "friend_id INTEGER NOT NULL,"
                                     "event_type INTEGER NOT NULL,"
                                     "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                                     "message BLOB)") ) {
            Utils::bail("unable to create messages table: " + createTableQuery.lastError().text());
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
        fWipeQuery = prepareQuery("DELETE FROM events WHERE (friend_id = :friend_id OR :friend_id2 < 0)");
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
