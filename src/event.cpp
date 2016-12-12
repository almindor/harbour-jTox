#include "event.h"
#include "utils.h"
#include <QDebug>

namespace JTOX {

    Event::Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString& message, qint64 sendID) :
        fID(id), fFriendID(friendID), fEventType(eventType), fMessage(message), fSendID(sendID)
    {
        createdAt.setTimeSpec(Qt::UTC);
        fCreated = createdAt.toLocalTime();
    }

    const QVariant Event::value(int role) const {
        switch ( role ) {
            case erID: return fID;
            case erCreated: return fCreated;
            case erEventType: return fEventType;
            case erMessage: return fMessage;
        }

        return QVariant("invalid_role");
    }

    quint32 Event::id() const {
        return fID;
    }

    qint64 Event::sendID() const
    {
        return fSendID;
    }

    EventType Event::type() const
    {
        return fEventType;
    }

    void Event::delivered() {
        if ( fEventType != etMessageOutPending ) {
            Utils::bail("Delivered called on non pending message. MessageID: " + QString::number(fID) + " event type: " + QString::number(fEventType));
        }

        fEventType = etMessageOut;
    }

    void Event::viewed()
    {
        if ( fEventType != etMessageInUnread ) {
            Utils::bail("Viewed called on non unread message. MessageID: " + QString::number(fID) + " event type: " + QString::number(fEventType));
        }

        fEventType = etMessageIn;
    }

}
