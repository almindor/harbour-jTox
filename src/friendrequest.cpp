#include "friendrequest.h"
#include "utils.h"
#include <QSettings>
#include <QDebug>

namespace JTOX {

    FriendRequest::FriendRequest(const QString& address, const QString& message, const QString& name) :
        fAddress(address), fMessage(message), fName(name), fID(-1)
    {

    }

    const QVariant FriendRequest::value(int role) const {
        switch ( role ) {
            case rrPublicKey: return fAddress;
            case rrMessage: return fMessage;
            case rrName: return fName;
        }

        Utils::bail("Invalid request role");
        return QVariant();
    }

    void FriendRequest::setName(const QString& name) {
        fName = name;
    }

    const QString FriendRequest::getName() const
    {
        return fName;
    }

    const QString FriendRequest::getAddress() const
    {
        return fAddress;
    }

    const QString FriendRequest::getMessage() const
    {
        return fMessage;
    }

    void FriendRequest::setID(int id)
    {
        if ( fID >= 0 ) {
            Utils::bail("Attempted to set request ID twice");
        }

        fID = id;
    }

    int FriendRequest::getID() const
    {
        return fID;
    }

}
