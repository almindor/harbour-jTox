#include "friendrequest.h"
#include "utils.h"
#include <QSettings>
#include <QDebug>

namespace JTOX {

    FriendRequest::FriendRequest(const QString& address, const QString& message, const QString& name) :
        fAddress(address), fMessage(message), fName(name)
    {

    }

    FriendRequest::FriendRequest(const QString& address) : fAddress(address)
    {
        QSettings settings;
        fMessage = settings.value("app/friends/requests/" + address + "/message").toString();
        fName = settings.value("app/friends/requests/" + address + "/name").toString();
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
        save();
    }

    void FriendRequest::save() {
        QSettings settings;
        settings.setValue("app/friends/requests/" + fAddress + "/address", fAddress);
        settings.setValue("app/friends/requests/" + fAddress + "/name", fName);
        settings.setValue("app/friends/requests/" + fAddress + "/message", fMessage);
    }

    void FriendRequest::remove()
    {
        QSettings settings;
        settings.remove("app/friends/requests/" + fAddress);
    }

}
