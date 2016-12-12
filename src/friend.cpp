#include "friend.h"
#include "utils.h"

namespace JTOX {

    Friend::Friend(ToxCore& toxCore, uint32_t friend_id) : fToxCore(toxCore), fFriendID(friend_id),
        fName(), fConnectionStatus(TOX_CONNECTION_NONE), fUserStatus(TOX_USER_STATUS_NONE),
        fStatusMessage(), fTyping(false), fPublicKey(), fUnviewed(false)
    {
        refresh();
    }

    void Friend::refresh() {
        if ( !fToxCore.getInitialized() ) {
            qDebug() << "Friend refresh called when toxcore not initialized!";
            return;
        }

        TOX_ERR_FRIEND_QUERY error;
        size_t friend_name_size = tox_friend_get_name_size(fToxCore.tox(), fFriendID, &error);
        if ( error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend name size");
        }

        uint8_t name[friend_name_size];
        if ( !tox_friend_get_name(fToxCore.tox(), fFriendID, name, &error) || error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend name");
        }
        fName = QString::fromUtf8((char*) name, friend_name_size);

        fTyping = tox_friend_get_typing(fToxCore.tox(), fFriendID, &error);
        if ( error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend typing status");
        }

        fConnectionStatus = tox_friend_get_connection_status(fToxCore.tox(), fFriendID, &error);
        if ( error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend connection status");
        }

        fUserStatus = tox_friend_get_status(fToxCore.tox(), fFriendID, &error);
        if ( error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend user status");
        }

        size_t friend_status_size = tox_friend_get_status_message_size(fToxCore.tox(), fFriendID, &error);
        if ( error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend status message size");
        }

        uint8_t status[friend_status_size];
        tox_friend_get_status_message(fToxCore.tox(), fFriendID, status, &error);
        if ( error != TOX_ERR_FRIEND_QUERY_OK ) {
            Utils::bail("Error retrieving friend status message");
        }
        fStatusMessage = QString::fromUtf8((char*) status, friend_status_size);

        TOX_ERR_FRIEND_GET_PUBLIC_KEY pubError;
        uint8_t pubRaw[TOX_PUBLIC_KEY_SIZE];
        tox_friend_get_public_key(fToxCore.tox(), fFriendID, pubRaw, &pubError);
        if ( pubError != TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK ) {
            Utils::bail("Error retrieving friend public key");
        }
        fPublicKey = Utils::key_to_hex(pubRaw, TOX_PUBLIC_KEY_SIZE);
    }

    QVariant Friend::value(int role) const {
        switch ( role ) {
            case frName: return name();
            case frStatus: return status();
            case frStatusMessage: return fStatusMessage;
            case frTyping: return fTyping;
            case frFriendID: return fFriendID;
            case frPublicKey: return fPublicKey;
            case frUnviewed: return fUnviewed;
        }

        Utils::bail("Invalid role requested for friend value");
        return QVariant();
    }

    quint32 Friend::friendID() const {
        return fFriendID;
    }

    const QString Friend::name() const
    {
        if ( !fName.isEmpty() ) {
            return fName;
        }

        return fPublicKey;
    }

    void Friend::setName(const QString& name)
    {
        fName = name;
    }

    int Friend::status() const {
        return Utils::get_overall_status(fConnectionStatus, fUserStatus);
    }

    void Friend::setStatus(int status)
    {
        fUserStatus = (TOX_USER_STATUS) status;
    }

    void Friend::setConStatus(int conStatus)
    {
        fConnectionStatus = (TOX_CONNECTION) conStatus;
    }

    void Friend::setStatusMessage(const QString& statusMessage)
    {
        fStatusMessage = statusMessage;
    }

    bool Friend::typing() const
    {
        return fTyping;
    }

    void Friend::setTyping(bool typing)
    {
        fTyping = typing;
    }

    bool Friend::unviewed() const
    {
        return fUnviewed;
    }

    void Friend::setViewed()
    {
        fUnviewed = false;
    }

    void Friend::setUnviewed()
    {
        fUnviewed = true;
    }

}
