#include "encryptsave.h"
#include "utils.h"
#include <QDebug>
#include <QCryptographicHash>

namespace JTOX {

    EncryptSave::EncryptSave() : fKey(NULL)
    {
    }

    EncryptSave::~EncryptSave() {
        if( fKey != NULL ) {
            tox_pass_key_free(fKey);
        }
    }

    void EncryptSave::setPassword(const QString& password, const QByteArray& data) {
        if ( fKey != NULL ) {
            tox_pass_key_free(fKey);
            fKey = NULL;
        }
        QByteArray salt(TOX_PASS_SALT_LENGTH, Qt::Uninitialized);

        // try to get salt if we have source encrypted data
        if ( isEncrypted(data) ) {
            TOX_ERR_GET_SALT saltError;
            tox_get_salt((uint8_t*) data.data(), (uint8_t*) salt.data(), &saltError);
            if ( saltError != TOX_ERR_GET_SALT_OK ) {
                qDebug() << "Unable to get salt from data\n";
                Utils::bail("Unable to get salt from data");
            }
        }

        TOX_ERR_KEY_DERIVATION error;
        size_t passSize = password.toUtf8().size();
        uint8_t* passRaw = (uint8_t*) password.toUtf8().data();
        fKey = tox_pass_key_new();
        tox_pass_key_derive_with_salt(fKey, passRaw, passSize, (uint8_t*) salt.data(), &error);

        if ( error != TOX_ERR_KEY_DERIVATION_OK ) {
            qDebug() << "Unable to derive key\n";
            Utils::bail("Unable to derive key");
        }
    }

    bool EncryptSave::isEncrypted(const QByteArray& data) const
    {
        bool encrypted = false;
        if ( data.size() > TOX_PASS_ENCRYPTION_EXTRA_LENGTH ) {
            encrypted = tox_is_data_encrypted((uint8_t*) data.data());
        }
        return encrypted;
    }

    const QByteArray EncryptSave::encryptRaw(const QByteArray& data) const {
        TOX_ERR_ENCRYPTION error;
        size_t rawSize = data.size();
        QByteArray rawResult(rawSize + TOX_PASS_ENCRYPTION_EXTRA_LENGTH, Qt::Uninitialized);

        tox_pass_key_encrypt(fKey, (uint8_t*) data.data(), rawSize, (uint8_t*) rawResult.data(), &error);

        if ( error != TOX_ERR_ENCRYPTION_OK ) {
            qDebug() << "Error on data encryption";
            Utils::bail("Error on data encryption");
        }

        return rawResult;
    }

    const QByteArray EncryptSave::encrypt(const QString& data) const {
        return encryptRaw(data.toUtf8());
    }

    const QString EncryptSave::decrypt(const QByteArray& data) {
        return decryptRaw(data);
    }

    const QByteArray EncryptSave::decryptRaw(const QByteArray& data, bool ignoreErrors) {
        TOX_ERR_DECRYPTION error;
        TOX_ERR_GET_SALT saltError;

        QByteArray salt(TOX_PASS_SALT_LENGTH, Qt::Uninitialized);
        tox_get_salt((uint8_t*) data.data(), (uint8_t*) salt.data(), &saltError);
        if ( saltError != TOX_ERR_GET_SALT_OK ) {
            Utils::bail("Error getting salt from encrypted data"); // not ignorable
        }

        QByteArray result(data.size() - TOX_PASS_ENCRYPTION_EXTRA_LENGTH, Qt::Uninitialized);
        tox_pass_key_decrypt(fKey, (uint8_t*) data.data(), data.size(), (uint8_t*) result.data(), &error);
        if ( error != TOX_ERR_DECRYPTION_OK ) {
            if ( ignoreErrors ) {
                return QByteArray();
            }
            Utils::bail("Decryption error");
        }

        return result;
    }

    bool EncryptSave::validateDecrypt(const QByteArray& data) {
        return decryptRaw(data, true).size() > 0;
    }

    bool EncryptSave::getPasswordIsSet() const
    {
        return fKey != NULL;
    }

}
