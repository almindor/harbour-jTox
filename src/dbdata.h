#ifndef DBDATA_H
#define DBDATA_H

#include "event.h"
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
        void getEvents(EventList& list, quint32 friendID);
        int getUnviewedEventCount(qint64 friendID);
        int insertEvent(qint64 sendID, quint32 friendID, EventType eventType, const QString& message, QDateTime& createdAt);
        void updateEventType(int id, EventType eventType);
        void deliverEvent(quint32 sendID, quint32 friendID);
        void wipe(qint64 friendID);
    private:
        EncryptSave& fEncryptSave;
        QSqlDatabase fDB;
        QSqlQuery fEventSelectQuery;
        QSqlQuery fLastEventSelectQuery;
        QSqlQuery fEventUnviewedCountQuery;
        QSqlQuery fEventInsertQuery;
        QSqlQuery fEventUpdateQuery;
        QSqlQuery fEventDeliveredQuery;
        QSqlQuery fWipeQuery;
        void createTables();
        void prepareQueries();
        const QSqlQuery prepareQuery(const QString& sql);
    };

}

#endif // DBDATA_H
