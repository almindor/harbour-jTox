#ifndef FRIENDREQUEST_H
#define FRIENDREQUEST_H

#include <QString>
#include <QVariant>
#include <QList>

namespace JTOX {

    enum RequestRole {
        rrPublicKey = Qt::UserRole + 1,
        rrMessage,
        rrName // reverse lookup name from toxme so we don't have to re-do
    };

    class FriendRequest
    {
    public:
        FriendRequest(const QString& address, const QString& message, const QString& name);
        FriendRequest(const QString& address);
        const QVariant value(int role) const;
        void setName(const QString& name);
        void save();
        void remove();
    private:
        QString fAddress;
        QString fMessage;
        QString fName;
    };

    typedef QList<FriendRequest> RequestList;

}

#endif // FRIENDREQUEST_H
