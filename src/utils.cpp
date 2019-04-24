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

#include "utils.h"
#include <QtGlobal>
#include <QDebug>
#include <sodium/utils.h>

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

namespace JTOX {

    //***********************JToxException************************//

    void JToxException::raise() const
    {
        throw *this;
    }

    JToxException* JToxException::clone() const
    {
        return new JToxException(*this);
    }

    //***************************Tools****************************//

    static void print_backtrace(FILE *outb = stderr)
    {
        void *stack[128];
        int stack_size = backtrace(stack, sizeof(stack) / sizeof(void *));
        char **stack_symbols = backtrace_symbols(stack, stack_size);
        fprintf(outb, "Stack [%d]:\n", stack_size);
        if(FILE *cppfilt = popen("c++filt", "rw")) {
            dup2(fileno(outb), fileno(cppfilt));
            for(int i = stack_size-1; i>=0; --i)
                fwrite(stack_symbols[i], 1, strlen(stack_symbols[i]), cppfilt);
            pclose(cppfilt);
        } else {
            for(int i = stack_size-1; i>=0; --i)
                fprintf(outb, "#%d  %p [%s]\n", i, stack[i], stack_symbols[i]);
        }
    }

    const QString bail(const QString& error, bool soft)
    {
        if ( soft ) {
            qWarning() << "FATAL ERROR: " << error.toUtf8().data() << "\n";
        } else {
            print_backtrace();
            qFatal("FATAL ERROR: %s\n", error.toUtf8().data());
        }

        return error;
    }

    //***************************Utils****************************//

    const QString Utils::key_to_hex(const uint8_t* key, int size) {
        char hexRaw[size * 2 + 1];
        if ( sodium_bin2hex(hexRaw, size * 2 + 1, key, size) == NULL ) {
            fatal("key_to_hex conversion failed");
        }
        return QString::fromUtf8(hexRaw, size * 2).toUpper();
    }

    void Utils::hex_to_key(const QString& hexKey, unsigned char* key) {
        const QByteArray hexBytes = hexKey.toUtf8();
        if ( sodium_hex2bin(key, TOX_PUBLIC_KEY_SIZE, hexBytes.data(), hexBytes.size(), NULL, NULL, NULL) != 0 ) {
            fatal("hex_to_key conversion failed");
        }
    }

    void Utils::hex_to_address(const QString &hexKey, unsigned char *address)
    {
        const QByteArray hexBytes = hexKey.toUtf8();
        if ( sodium_hex2bin(address, TOX_ADDRESS_SIZE, hexBytes.data(), hexBytes.size(), NULL, NULL, NULL) != 0 ) {
            fatal("hex_to_address conversion failed");
        }
    }

    /* Amalgamated status
     * 0 - offline
     * 1 - online/ready
     * 2 - online/away
     * 3 - online/busy
     */
    int Utils::get_overall_status(TOX_CONNECTION conStatus, TOX_USER_STATUS userStatus) {
        if ( conStatus == TOX_CONNECTION_NONE ) {
            return 0;
        }

        switch ( userStatus ) {
            case TOX_USER_STATUS_NONE: return 1;
            case TOX_USER_STATUS_AWAY: return 2;
            case TOX_USER_STATUS_BUSY: return 3;
        }

        fatal("Unknown user status");
        return -1;
    }

    void abortHandler( int signum )
    {
       // associate each signal with a signal name string.
       const char* name = NULL;
       switch( signum )
       {
       case SIGABRT: name = "SIGABRT";  break;
       case SIGSEGV: name = "SIGSEGV";  break;
       case SIGBUS:  name = "SIGBUS";   break;
       case SIGILL:  name = "SIGILL";   break;
       case SIGFPE:  name = "SIGFPE";   break;
       }

       // Notify the user which signal was caught. We use printf, because this is the
       // most basic output function. Once you get a crash, it is possible that more
       // complex output systems like streams and the like may be corrupted. So we
       // make the most basic call possible to the lowest level, most
       // standard print function.
       if ( name )
          fprintf( stderr, "Caught signal %d (%s)\n", signum, name );
       else
          fprintf( stderr, "Caught signal %d\n", signum );

       // Dump a stack trace.
       // This is the function we will be implementing next.
       print_backtrace();

       // If you caught one of the above signals, it is likely you just
       // want to quit your program right now.
       exit( signum );
    }

    void register_signals()
    {
        signal( SIGABRT, abortHandler );
        signal( SIGSEGV, abortHandler );
        signal( SIGILL,  abortHandler );
        signal( SIGFPE,  abortHandler );
    }

    const QString Utils::warn(const QString &error)
    {
        return bail(error, true);
    }

    const QString Utils::fatal(const QString &error)
    {
        return bail(error, false);
    }

    quint64 Utils::transferID(quint32 friend_id, quint32 file_number)
    {
        return (quint64)friend_id << 32 | file_number; // combined unique ID for transfer
    }

    quint32 Utils::friendID(quint64 transferID)
    {
        return (quint32) (transferID >> 32);
    }

    quint32 Utils::fileNumber(quint64 transferID)
    {
        return (quint32) transferID;
    }

    const StringListUTF8 Utils::splitStringUTF8(const QByteArray &source, int maxByteSize)
    {
        StringListUTF8 result;
        int i = 0;
        int size;

        while ( i < source.size() ) {
            size = maxByteSize;
            while ( i + size < source.size() &&
                    (source.at(i + size) & 0xC0) == 0x80 ) {
                --size; // find UTF-8 character start to the left
            }

            result << source.mid(i, size);
            i += size;
        }
        // result << source.mid(i);

        return result;
    }

    const QString Utils::handleFileControlError(TOX_ERR_FILE_CONTROL error, bool soft)
    {
        switch ( error ) {
            case TOX_ERR_FILE_CONTROL_ALREADY_PAUSED: return bail("File transfer already paused", soft);
            case TOX_ERR_FILE_CONTROL_DENIED: return bail("Permission denied", soft);
            case TOX_ERR_FILE_CONTROL_FRIEND_NOT_CONNECTED: return bail("Friend not connected", soft);
            case TOX_ERR_FILE_CONTROL_FRIEND_NOT_FOUND: return bail("Friend not found", soft);
            case TOX_ERR_FILE_CONTROL_NOT_FOUND: return bail("File transfer not found", soft);
            case TOX_ERR_FILE_CONTROL_NOT_PAUSED: return bail("File transfer not paused", soft);
            case TOX_ERR_FILE_CONTROL_SENDQ: return bail("File transfer send queue full", soft);
            case TOX_ERR_FILE_CONTROL_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleFileSendChunkError(TOX_ERR_FILE_SEND_CHUNK error, bool soft)
    {
        switch ( error ) {
            case TOX_ERR_FILE_SEND_CHUNK_FRIEND_NOT_CONNECTED: return bail("Friend not connected", soft);
            case TOX_ERR_FILE_SEND_CHUNK_FRIEND_NOT_FOUND: return bail("Friend not found", soft);
            case TOX_ERR_FILE_SEND_CHUNK_INVALID_LENGTH: return bail("Invalid chunk length", soft);
            case TOX_ERR_FILE_SEND_CHUNK_NOT_FOUND: return bail("Transfer not found", soft);
            case TOX_ERR_FILE_SEND_CHUNK_NOT_TRANSFERRING: return bail("Invalid transfer state", soft);
            case TOX_ERR_FILE_SEND_CHUNK_NULL: return bail("Invalid null parameter", soft);
            case TOX_ERR_FILE_SEND_CHUNK_SENDQ: return bail("File transfer send queue full", soft);
            case TOX_ERR_FILE_SEND_CHUNK_WRONG_POSITION: return bail("File transfer wrong position", soft);

            case TOX_ERR_FILE_SEND_CHUNK_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleSendMessageError(TOX_ERR_FRIEND_SEND_MESSAGE error, bool soft)
    {
        switch ( error ) {
            case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY: return bail("Cannot send empty message", soft);
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED: return bail("Cannot send message, friend not online", soft);
            case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND: return bail("Cannot send message, friend not found", soft);
            case TOX_ERR_FRIEND_SEND_MESSAGE_NULL: return bail("Cannot send null message", soft);
            case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ: return bail("Cannot send message, sendq error", soft);
            case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG: return bail("Cannot send message it is too long", soft);
            case TOX_ERR_FRIEND_SEND_MESSAGE_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxFileSendError(TOX_ERR_FILE_SEND error)
    {
        switch ( error ) {
            case TOX_ERR_FILE_SEND_FRIEND_NOT_CONNECTED: return Utils::fatal("Cannot send file, friend not connected");
            case TOX_ERR_FILE_SEND_FRIEND_NOT_FOUND: return Utils::fatal("Cannot send file, friend not found");
            case TOX_ERR_FILE_SEND_NAME_TOO_LONG: return Utils::fatal("Cannot send file, name too long");
            case TOX_ERR_FILE_SEND_NULL: return Utils::fatal("Cannot send file, unexpected null argument");
            case TOX_ERR_FILE_SEND_TOO_MANY: return Utils::fatal("Cannot send file, too many concurrent transfers in progress");
            case TOX_ERR_FILE_SEND_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxNewError(TOX_ERR_NEW error)
    {
        switch ( error ) {
            case TOX_ERR_NEW_NULL: return Utils::fatal("Cannot create toxcore, null argument");
            case TOX_ERR_NEW_MALLOC: return Utils::fatal("Cannot create toxcore, malloc error");
            case TOX_ERR_NEW_PORT_ALLOC: return Utils::fatal("Cannot create toxcore, port bind permission error");
            case TOX_ERR_NEW_PROXY_BAD_TYPE: return Utils::fatal("Cannot create toxcore, invalid proxy type");
            case TOX_ERR_NEW_PROXY_BAD_HOST: return Utils::fatal("Cannot create toxcore, bad proxy host");
            case TOX_ERR_NEW_PROXY_BAD_PORT: return Utils::fatal("Cannot create toxcore, bad proxy port");
            case TOX_ERR_NEW_PROXY_NOT_FOUND: return Utils::fatal("Cannot create toxcore, proxy not found");
            case TOX_ERR_NEW_LOAD_ENCRYPTED: return Utils::fatal("Cannot create toxcore, save data encrypted");
            case TOX_ERR_NEW_LOAD_BAD_FORMAT: return Utils::fatal("Cannot create toxcore, save data invalid");
            case TOX_ERR_NEW_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxAVNewError(TOXAV_ERR_NEW error)
    {
        switch ( error ) {
            case TOXAV_ERR_NEW_NULL: return Utils::fatal("[A/V] One of the arguments to the function was NULL when it was not expected");
            case TOXAV_ERR_NEW_MALLOC: return Utils::fatal("[A/V] Memory allocation failure while trying to allocate structures required for the A/V session");
            case TOXAV_ERR_NEW_MULTIPLE: return Utils::fatal("[A/V] Attempted to create a second session for the same Tox instance");
            case TOXAV_ERR_NEW_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxAVAnswerError(TOXAV_ERR_ANSWER error)
    {
        switch ( error ) {
            case TOXAV_ERR_ANSWER_FRIEND_NOT_CALLING: return bail("[A/V] Friend is not calling", true);
            case TOXAV_ERR_ANSWER_FRIEND_NOT_FOUND: return bail("[A/V] Friend not found", true);
            case TOXAV_ERR_ANSWER_CODEC_INITIALIZATION: return bail("[A/V] Unable to initialize codec", true);
            case TOXAV_ERR_ANSWER_INVALID_BIT_RATE: return bail("[A/V] Invalid bitrate", true);
            case TOXAV_ERR_ANSWER_SYNC: return bail("[A/V] Synchronization error", true);
            case TOXAV_ERR_ANSWER_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxAVControlError(TOXAV_ERR_CALL_CONTROL error)
    {
        switch ( error ) {
            case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_IN_CALL: return bail("[A/V] Friend is not calling", true);
            case TOXAV_ERR_CALL_CONTROL_FRIEND_NOT_FOUND: return bail("[A/V] Friend not found", true);
            case TOXAV_ERR_CALL_CONTROL_INVALID_TRANSITION: return bail("[A/V] Invalid state transition", true);
            case TOXAV_ERR_CALL_CONTROL_SYNC: return bail("[A/V] Synchronization error", true);
            case TOXAV_ERR_CALL_CONTROL_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxAVCallError(TOXAV_ERR_CALL error)
    {
        switch ( error ) {
            case TOXAV_ERR_CALL_FRIEND_NOT_CONNECTED: return bail("[A/V] Friend not connected", true);
            case TOXAV_ERR_CALL_FRIEND_NOT_FOUND: return bail("[A/V] Friend not found", true);
            case TOXAV_ERR_CALL_FRIEND_ALREADY_IN_CALL: return bail("[A/V] Friend already in call", true);
            case TOXAV_ERR_CALL_MALLOC: return bail("[A/V] Memory allocation error", true);
            case TOXAV_ERR_CALL_INVALID_BIT_RATE: return bail("[A/V] Invalid bitrate", true);
            case TOXAV_ERR_CALL_SYNC: return bail("[A/V] Synchronization error", true);
            case TOXAV_ERR_CALL_OK: return QString();
        }

        return fatal("Unknown error");
    }

    const QString Utils::handleToxAVSendError(TOXAV_ERR_SEND_FRAME error)
    {
        switch (error) {
            case TOXAV_ERR_SEND_FRAME_FRIEND_NOT_FOUND: return bail("[A/V] Friend not found", true);
            case TOXAV_ERR_SEND_FRAME_FRIEND_NOT_IN_CALL: return bail("[A/V] Friend not in call", true);
            case TOXAV_ERR_SEND_FRAME_INVALID: return bail("[A/V] Frame invalid", true);
            case TOXAV_ERR_SEND_FRAME_NULL: return bail("[A/V] Frame null", true);
            case TOXAV_ERR_SEND_FRAME_PAYLOAD_TYPE_DISABLED: return bail("[A/V] Payload type disabled", true);
            case TOXAV_ERR_SEND_FRAME_RTP_FAILED: return bail("[A/V] RTP failed", true);
            case TOXAV_ERR_SEND_FRAME_SYNC: return bail("[A/V] Synchronization error", true);
            case TOXAV_ERR_SEND_FRAME_OK: return QString();
        }

        return fatal("Unknown error");
    }


}
