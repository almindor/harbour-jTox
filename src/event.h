#ifndef EVENT_H
#define EVENT_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QList>
#include "encryptsave.h"

namespace JTOX {

    enum EventType {
        etEdit = 0,
        etMessageOut = 1,
        etMessageInUnread = 2,
        etMessageOutPending = 3,
        etMessageIn = 4,
        etMessageOutOffline = 5, // pending to be sent
        etCallOutRejected = 6,
        etCallOutAccepted = 7,
        etCallInRejected = 8,
        etCallInAccepted = 9,
        etFileTransferSent = 10,
        etFileTransferReceived = 11
    };

    enum EventRole
    {
        erID = Qt::UserRole + 1,
        erEventType,
        erCreated,
        erMessage
    };

    class Event
    {
    public:
        Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString& message, qint64 sendID);
        const QVariant value(int role) const;
        void delivered();
        void viewed();
        int id() const;
        const QString message() const;
        qint64 sendID() const;
        EventType type() const;
        void setSendID(qint64 sendID);
        void setEventType(EventType eventType);
    private:
        int fID;
        quint32 fFriendID;
        EventType fEventType;
        QString fMessage;
        qint64 fSendID;
        QDateTime fCreated;

        const QString hyperLink(const QString& message) const;
        const QString fileInfo(const QString& message) const;
        const QString wrapMessage() const;
    };

    typedef QList<Event> EventList;

}

#endif // EVENT_H
