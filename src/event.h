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
        etMessageOut,
        etMessageInUnread,
        etMessageOutPending,
        etMessageIn,
        etCallOutRejected,
        etCallOutAccepted,
        etCallInRejected,
        etCallInAccepted,
        etFileTransfer
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
        quint32 id() const;
        qint64 sendID() const;
        EventType type() const;
    private:
        int fID;
        quint32 fFriendID;
        EventType fEventType;
        QString fMessage;
        qint64 fSendID;
        QDateTime fCreated;
    };

    typedef QList<Event> EventList;

}

#endif // EVENT_H
