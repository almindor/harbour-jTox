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

    //***************************Utils****************************//

    const QString Utils::key_to_hex(const uint8_t* key, int size) {
        char hexRaw[size * 2 + 1];
        if ( sodium_bin2hex(hexRaw, size * 2 + 1, key, size) == NULL ) {
            Utils::bail("key_to_hex conversion failed");
        }
        return QString::fromUtf8(hexRaw, size * 2).toUpper();
    }

    void Utils::hex_to_key(const QString& hexKey, unsigned char* key) {
        const QByteArray hexBytes = hexKey.toUtf8();
        if ( sodium_hex2bin(key, TOX_PUBLIC_KEY_SIZE, hexBytes.data(), hexBytes.size(), NULL, NULL, NULL) != 0 ) {
            Utils::bail("hex_to_key conversion failed");
        }
    }

    void Utils::hex_to_address(const QString &hexKey, unsigned char *address)
    {
        const QByteArray hexBytes = hexKey.toUtf8();
        if ( sodium_hex2bin(address, TOX_ADDRESS_SIZE, hexBytes.data(), hexBytes.size(), NULL, NULL, NULL) != 0 ) {
            Utils::bail("hex_to_address conversion failed");
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

        Utils::bail("Unknown user status");
        return -1;
    }

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

    bool Utils::bail(const QString& error, bool soft)
    {
        if ( soft ) {
            qDebug() << "FATAL ERROR: " << error.toUtf8().data() << "\n";
            return true;
        }

        print_backtrace();
        qFatal("FATAL ERROR: %s\n", error.toUtf8().data());
        return false;
    }

}
