/*
    Copyright (C) 2016 Ales Katona.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "toxcore.h"
#include "utils.h"
#include <QObject>
#include <QString>
#include <QDebug>

namespace JTOX {

    void c_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data)
    {
        //qDebug() << "c_connection_status_cb\n";
        Q_UNUSED(tox);
        Q_UNUSED(connection_status);
        ToxCore* jTox = (ToxCore*) user_data;
        jTox->setConnectionStatus();
    }

    void c_friend_request_cb(Tox *tox, const uint8_t *public_key, const uint8_t *message,
                             size_t length, void *user_data)
    {
        //qDebug() << "c_friend_request_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;
        QString msg;
        if ( message != NULL && length > 0 ) {
            msg = QString::fromUtf8((char*)message, length);
        }
        const QByteArray pk((char*) public_key, TOX_PUBLIC_KEY_SIZE);
        const QString hexKey = pk.toHex();

        jTox->onFriendRequest(hexKey, msg);
    }

    void c_friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message,
                             size_t length, void *user_data)
    {
        //qDebug() << "c_friend_message_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onMessageReceived(friend_number, type, QString::fromUtf8((char*)message, length));
    }

    void c_friend_connection_status_cb(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status,
                                       void *user_data) {
        //qDebug() << "c_friend_connection_status_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFriendConStatusChanged(friend_number, connection_status);
    }

    void c_friend_name_cb(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data) {
        //qDebug() << "c_friend_name_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFriendNameChanged(friend_number, QString::fromUtf8((char*) name, length));
    }

    void c_friend_status_cb(Tox *tox, uint32_t friend_number, TOX_USER_STATUS status, void *user_data) {
        //qDebug() << "c_friend_status_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFriendStatusChanged(friend_number, status);
    }

    void c_friend_status_message_cb(Tox *tox, uint32_t friend_number, const uint8_t *message, size_t length,
            void *user_data) {
        //qDebug() << "c_friend_status_message_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFriendStatusMsgChanged(friend_number, QString::fromUtf8((char*) message, length));
    }

    void c_friend_typing_cb(Tox *tox, uint32_t friend_number, bool is_typing, void *user_data) {
        //qDebug() << "c_friend_typing_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFriendTypingChanged(friend_number, is_typing);
    }

    void c_friend_read_receipt_cb(Tox *tox, uint32_t friend_number, uint32_t message_id, void *user_data) {
        //qDebug() << "c_friend_read_receipt_cb\n";
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onMessageDelivered(friend_number, message_id);
    }

}
