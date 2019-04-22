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
#include "toxcoreav.h"
#include "workerav.h"
#include "utils.h"
#include <QObject>
#include <QString>
#include <QDebug>

namespace JTOX {

    // TOXCORE

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

    void c_tox_file_recv_control_cb(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                    TOX_FILE_CONTROL control, void *user_data) {
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        switch ( control ) {
            case TOX_FILE_CONTROL_CANCEL: return jTox->onFileCanceled(friend_number, file_number);
            case TOX_FILE_CONTROL_PAUSE: return jTox->onFilePaused(friend_number, file_number);
            case TOX_FILE_CONTROL_RESUME: return jTox->onFileResumed(friend_number, file_number);
            default: return;
        }
    }

    void c_tox_file_recv_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size,
                            const uint8_t *filename, size_t filename_length, void *user_data) {
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        if ( kind == TOX_FILE_KIND_AVATAR ) {
            TOX_ERR_FILE_GET error;
            QByteArray fileID(TOX_FILE_ID_LENGTH, 0);
            uint8_t* file_id = (quint8*) fileID.data();
            tox_file_get_file_id(tox, friend_number, file_number, file_id, &error);
            if ( error != TOX_ERR_FILE_GET_OK ) {
                TOX_ERR_FILE_CONTROL ctrl_error;
                tox_file_control(tox, friend_number, file_number, TOX_FILE_CONTROL_CANCEL, &ctrl_error);
                if ( ctrl_error != TOX_ERR_FILE_CONTROL_OK ) {
                    qDebug() << "Error canceling avatar request: " << ctrl_error << "\n";
                }
                return;
            }

            jTox->onAvatarFileReceived(friend_number, file_number, file_size, fileID);
        } else { // std. file
            if ( filename_length > TOX_MAX_FILENAME_LENGTH ) { // probably not required but safer
                filename_length = TOX_MAX_FILENAME_LENGTH;
            }

            const QString fn = QString::fromUtf8((char*)filename, filename_length);
            jTox->onFileReceived(friend_number, file_number, file_size, fn);
        }
    }

    void c_tox_file_recv_chunk_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                  const uint8_t *data, size_t length, void *user_data) {
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFileChunkReceived(friend_number, file_number, position, data, length);
    }

    void c_tox_file_chunk_request_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                     size_t length, void *user_data) {
        Q_UNUSED(tox);
        ToxCore* jTox = (ToxCore*) user_data;

        jTox->onFileChunkRequest(friend_number, file_number, position, length);
    }

    // TOXAV

    void c_toxav_call_cb(ToxAV *av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data) {
        Q_UNUSED(av);
        ToxCoreAV* jToxAV = (ToxCoreAV*) user_data;

        jToxAV->onIncomingCall(friend_number, audio_enabled, video_enabled);
    }

    void c_toxav_call_state_cb(ToxAV *av, uint32_t friend_number, uint32_t state, void *user_data) {
        Q_UNUSED(av);
        ToxCoreAV* jToxAV = (ToxCoreAV*) user_data;

        jToxAV->onCallStateChanged(friend_number, state);
    }

    void c_toxav_audio_bit_rate_cb(ToxAV* av, uint32_t friend_number, uint32_t audio_bit_rate, void* user_data)
    {
        Q_UNUSED(av);
        ToxCoreAV* jToxAV = (ToxCoreAV*) user_data;

        qDebug() << "c_toxav_audio_bit_rate_cb\n";
    }

    void c_toxav_audio_receive_frame_cb(ToxAV* av, uint32_t friend_number, const int16_t* pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate, void* user_data)
    {
        Q_UNUSED(av);
        WorkerToxAVIterator* worker = (WorkerToxAVIterator*) user_data;

        worker->onAudioFrameReceived(friend_number, pcm, sample_count, channels, sampling_rate);
    }

}
