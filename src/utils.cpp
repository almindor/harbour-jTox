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
#include <sodium/utils.h>

#include <execinfo.h>
#include <errno.h>
#include <stdio.h>
#include <cxxabi.h>
#include <signal.h>

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

    void Utils::bail(const QString& error)
    {
        qFatal("FATAL ERROR: %s\n", error.toUtf8().data());
    }

    static inline void printStackTrace( FILE *out = stderr, unsigned int max_frames = 63 )
    {
       fprintf(out, "stack trace:\n");

       // storage array for stack trace address data
       void* addrlist[max_frames+1];

       // retrieve current stack addresses
       unsigned int addrlen = backtrace( addrlist, sizeof( addrlist ) / sizeof( void* ));

       if ( addrlen == 0 )
       {
          fprintf( out, "  \n" );
          return;
       }

       // resolve addresses into strings containing "filename(function+address)",
       // Actually it will be ## program address function + offset
       // this array must be free()-ed
       char** symbollist = backtrace_symbols( addrlist, addrlen );

       size_t funcnamesize = 1024;
       char funcname[1024];

       // iterate over the returned symbol lines. skip the first, it is the
       // address of this function.
       for ( unsigned int i = 4; i < addrlen; i++ )
       {
          char* begin_name   = NULL;
          char* begin_offset = NULL;
          char* end_offset   = NULL;

          // find parentheses and +address offset surrounding the mangled name
          // not OSX style
          // ./module(function+0x15c) [0x8048a6d]
          for ( char *p = symbollist[i]; *p; ++p )
          {
             if ( *p == '(' )
                begin_name = p;
             else if ( *p == '+' )
                begin_offset = p;
             else if ( *p == ')' && ( begin_offset || begin_name ))
                end_offset = p;
          }

          if ( begin_name && end_offset && ( begin_name < end_offset ))
          {
             *begin_name++   = '\0';
             *end_offset++   = '\0';
             if ( begin_offset )
                *begin_offset++ = '\0';

             // mangled name is now in [begin_name, begin_offset) and caller
             // offset in [begin_offset, end_offset). now apply
             // __cxa_demangle():

             int status = 0;
             char* ret = abi::__cxa_demangle( begin_name, funcname,
                                              &funcnamesize, &status );
             char* fname = begin_name;
             if ( status == 0 )
                fname = ret;

             if ( begin_offset )
             {
                fprintf( out, "  %-30s ( %-40s  + %-6s) %s\n",
                         symbollist[i], fname, begin_offset, end_offset );
             } else {
                fprintf( out, "  %-30s ( %-40s    %-6s) %s\n",
                         symbollist[i], fname, "", end_offset );
             }
          } else {
             // couldn't parse the line? print the whole line.
             fprintf(out, "  %-40s\n", symbollist[i]);
          }
       }

       free(symbollist);
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
       printStackTrace();

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

}
