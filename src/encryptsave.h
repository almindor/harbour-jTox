#ifndef ENCRYPTSAVE_H
#define ENCRYPTSAVE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <tox/toxencryptsave.h>

namespace JTOX {

    class EncryptSave
    {
    public:
        EncryptSave();
        virtual ~EncryptSave();
        const QByteArray encrypt(const QString& data) const;
        const QByteArray encryptRaw(const QByteArray& data) const;
        const QString decrypt(const QByteArray& data);
        const QByteArray decryptRaw(const QByteArray& data, bool ignoreErrors = false);
        bool validateDecrypt(const QByteArray& data);
        bool getPasswordIsSet() const;
        void setPassword(const QString& password, const QByteArray& data = QByteArray());
        bool isEncrypted(const QByteArray& data) const;
    private:
        Tox_Pass_Key* fKey;
    };

}

#endif // ENCRYPTSAVE_H
