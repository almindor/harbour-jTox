#ifndef FRIEND_H
#define FRIEND_H

#include <QList>
#include <QVariant>
#include <tox/tox.h>
#include "toxcore.h"

namespace JTOX {

    enum FriendRoles
    {
        frName = Qt::UserRole + 1,
        frStatus,
        frStatusMessage,
        frTyping,
        frFriendID,
        frPublicKey,
        frUnviewed
    };

    class Friend
    {
    public:
        Friend(ToxCore& toxCore, uint32_t friend_id);
        void refresh();
        QVariant value(int role) const;
        quint32 friendID() const;
        const QString name() const;
        const QString address() const;
        void setName(const QString& name);
        void setOfflineName(const QString& name);
        int status() const;
        void setStatus(int status);
        void setConStatus(int conStatus);
        void setStatusMessage(const QString& statusMessage);
        bool typing() const;
        void setTyping(bool typing);

        void setViewed();
        void setUnviewed();
        bool unviewed() const;
    private:
        ToxCore& fToxCore;
        quint32 fFriendID;
        QString fName;
        TOX_CONNECTION fConnectionStatus;
        TOX_USER_STATUS fUserStatus;
        QString fStatusMessage;
        bool fTyping;
        QString fPublicKey;
        bool fUnviewed;
        QString fOfflineName;
    };

    typedef QList<Friend> FriendList;

}

#endif // FRIEND_H
