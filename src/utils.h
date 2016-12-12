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

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <stdint.h>
#include <tox/tox.h>
#include <QException>

namespace JTOX {

    class JToxException : public QException
    {
    public:
        void raise() const;
        JToxException *clone() const;
    };

    class Utils
    {
    public:
        static const QString key_to_hex(const uint8_t* key, int size);
        static void hex_to_key(const QString& hexKey, unsigned char* key);
        static void hex_to_address(const QString& hexKey, unsigned char* address);
        static int get_overall_status(TOX_CONNECTION conStatus, TOX_USER_STATUS userStatus);
        static void bail(const QString& error);
    };

    void register_signals();
}

#endif // UTILS_H
