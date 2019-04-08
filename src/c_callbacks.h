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

#ifndef C_CALLBACKS_H
#define C_CALLBACKS_H

#include <tox/tox.h>

namespace JTOX {
    // TOXCORE
    void c_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data);

    void c_friend_request_cb(Tox *tox, const uint8_t *public_key, const uint8_t *message,
                             size_t length, void *user_data);

    void c_friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message,
                             size_t length, void *user_data);

    void c_friend_connection_status_cb(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status,
            void *user_data);

    void c_friend_name_cb(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data);

    void c_friend_status_cb(Tox *tox, uint32_t friend_number, TOX_USER_STATUS status, void *user_data);

    void c_friend_status_message_cb(Tox *tox, uint32_t friend_number, const uint8_t *message, size_t length,
            void *user_data);

    void c_friend_typing_cb(Tox *tox, uint32_t friend_number, bool is_typing, void *user_data);

    void c_friend_read_receipt_cb(Tox *tox, uint32_t friend_number, uint32_t message_id, void *user_data);

    void c_tox_file_recv_control_cb(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                    TOX_FILE_CONTROL control, void *user_data);

    void c_tox_file_recv_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size,
                            const uint8_t *filename, size_t filename_length, void *user_data);

    void c_tox_file_recv_chunk_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                  const uint8_t *data, size_t length, void *user_data);

    void c_tox_file_chunk_request_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position,
                                     size_t length, void *user_data);

    // TOXAV
    void c_toxav_call_cb(ToxAV *av, uint32_t friend_number, bool audio_enabled, bool video_enabled, void *user_data);

    void c_toxav_call_state_cb(ToxAV *av, uint32_t friend_number, uint32_t state, void *user_data);

    void c_toxav_audio_bit_rate_cb(ToxAV *av, uint32_t friend_number, uint32_t audio_bit_rate, void *user_data);

    void c_toxav_audio_receive_frame_cb(ToxAV *av, uint32_t friend_number, const int16_t *pcm, size_t sample_count,
                                        uint8_t channels, uint32_t sampling_rate, void *user_data);

}

#endif // C_CALLBACKS_H
