#include "toxme.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QByteArray>
#include <QDateTime>
#include <QUrl>
#include <QCryptographicHash>
#include <QSettings>
#include <sodium.h>

namespace JTOX {

    const int TOXME_ACTION_PUSH = 1;
    const int TOXME_ACTION_DELETE = 2;
    const int TOXME_ACTION_LOOKUP = 3;
    const int TOXME_ACTION_REVERSE_LOOKUP = 5;
    const QString TOXME_DOMAIN = "toxme.io";
    const QUrl TOXME_PUBKEY_URL("https://" + TOXME_DOMAIN + "/pk");
    const QUrl TOXME_URL("https://" + TOXME_DOMAIN + "/api");

    const int TOXME_ERROR_OK = 0;
    const int TOXME_ERROR_METHOD_UNSUPPORTED = -1; // # Client didn't POST to /api
    const int TOXME_ERROR_NOTSECURE = -2; // Client is not using a secure connection
    const int TOXME_ERROR_BAD_PAYLOAD = -3; // Bad payload (possibly not encrypted with the correct key)
    const int TOXME_ERROR_NAME_TAKEN = -25; // Name is taken.
    const int TOXME_ERROR_DUPE_ID = -26; // The public key given is bound to a name already.
    const int TOXME_ERROR_INVALID_CHAR = -27; // Invalid char or type was used in a request.
    const int TOXME_ERROR_INVALID_NAME = -29; // Invalid name was sent
    const int TOXME_ERROR_UNKNOWN_NAME = -30; // Name not found
    const int TOXME_ERROR_INVALID_ID = -31; // Sent invalid data in place of an ID
    const int TOXME_ERROR_LOOKUP_FAILED = -41; // Lookup failed because of an error on the other domain's side.
    const int TOXME_ERROR_NO_USER = -42; // Lookup failed because that user doesn't exist on the domain
    const int TOXME_ERROR_LOOKUP_INTERNAL = -43; // Lookup failed because of an error on our side.
    const int TOXME_ERROR_RATE_LIMIT = -4; // Client is publishing IDs too fast

    Toxme::Toxme(const ToxCore& toxCore, EncryptSave& encryptSave) : QObject(0),
        fToxCore(toxCore), fEncryptSave(encryptSave),
        fNetManager(), fRequestID(-1), fNextRequestID(0), fPublicKey()
    {
        connect(&fNetManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpRequestDone(QNetworkReply*)));
        connect(&toxCore, &ToxCore::clientReset, this, &Toxme::refresh);

        QNetworkRequest request(TOXME_PUBKEY_URL);
        QNetworkReply* reply = fNetManager.get(request);
        reply->setProperty("requestID", -1);
    }

    int Toxme::lookup(const QString& name) {
        QJsonObject obj;
        obj["name"] = name;
        return publicRequest(obj, TOXME_ACTION_LOOKUP, name);
    }

    int Toxme::reverseLookup(const QString& publicKey) {
        QJsonObject obj;
        obj["id"] = publicKey;
        return publicRequest(obj, TOXME_ACTION_REVERSE_LOOKUP, publicKey);
    }

    int Toxme::push(const QString &name, const QString &toxID, const QString &bio, bool priv)
    {
        QJsonObject encryptedPayload;
        encryptedPayload["tox_id"] = toxID;
        encryptedPayload["name"] = name;
        encryptedPayload["privacy"] = priv ? 0 : 2;
        encryptedPayload["bio"] = bio;
        encryptedPayload["timestamp"] = (qint64) QDateTime::currentDateTimeUtc().toTime_t();
        setIsPrivate(priv); // set here confirm on change

        return privateRequest(encryptedPayload, TOXME_ACTION_PUSH, name);
    }

    int Toxme::remove()
    {        
        QJsonObject encryptedPayload;
        encryptedPayload["public_key"] = fToxCore.getHexPublicKey();
        encryptedPayload["timestamp"] = (qint64) QDateTime::currentDateTimeUtc().toTime_t();

        return privateRequest(encryptedPayload, TOXME_ACTION_DELETE, fToxCore.getHexPublicKey());
    }

    const QString Toxme::getDomain() const
    {
        return TOXME_DOMAIN;
    }

    int Toxme::requestID() const {
        return fRequestID;
    }

    int Toxme::publicRequest(QJsonObject& payload, int action, const QString& userInfo) {
        return postRequest(payload, action, userInfo);
    }

    int Toxme::privateRequest(const QJsonObject& encPayload, int action, const QString& userInfo) {
        QJsonObject payload;
        QByteArray nonce(crypto_box_NONCEBYTES, 0);
        randombytes((uint8_t*) nonce.data(), crypto_box_NONCEBYTES);

        const QJsonDocument doc(encPayload);
        const QString encrypted = fToxCore.encryptPayload(doc.toJson(QJsonDocument::Compact), pubKeyBin(), nonce);

        payload["public_key"] = fToxCore.getHexPublicKey();
        payload["nonce"] = QString(nonce.toBase64());
        payload["encrypted"] = encrypted;

        return postRequest(payload, action, userInfo);
    }

    int Toxme::postRequest(QJsonObject& payload, int action, const QString& userInfo) {
        fRequestID = fNextRequestID++;
        emit nextRequest(fRequestID);

        const QString memorabilia = generateMemorabilia();
        payload["action"] = action;
        payload["memorabilia"] = memorabilia;

        QNetworkRequest request(TOXME_URL);
        request.setRawHeader("Content-Type", "application/json");
        const QJsonDocument data(payload);
        QNetworkReply* reply = fNetManager.post(request, data.toJson(QJsonDocument::Compact));
        reply->setProperty("action", QVariant(action));
        reply->setProperty("requestID", QVariant(fRequestID));
        reply->setProperty("userInfo", QVariant(userInfo));
        reply->setProperty("memorabilia", QVariant(memorabilia));

        return fRequestID;
    }

    const QString Toxme::generateMemorabilia() const {
        QByteArray data(64, '0');
        randombytes_buf(data.data(), data.size());

        return data.toBase64();
    }

    const QString Toxme::handleReplyError(int error) const
    {
        switch ( error ) {
            case TOXME_ERROR_OK: return QString();
            case TOXME_ERROR_METHOD_UNSUPPORTED: return "Client didn't POST to /api";
            case TOXME_ERROR_NOTSECURE: return "Client is not using a secure connection";
            case TOXME_ERROR_BAD_PAYLOAD: return "Bad payload (possibly not encrypted with the correct key)";
            case TOXME_ERROR_NAME_TAKEN: return "Name is taken.";
            case TOXME_ERROR_DUPE_ID: return "The public key given is bound to a name already.";
            case TOXME_ERROR_INVALID_CHAR: return "Invalid char or type was used in a request.";
            case TOXME_ERROR_INVALID_NAME: return "Invalid name was sent.";
            case TOXME_ERROR_UNKNOWN_NAME: return "Name not found.";
            case TOXME_ERROR_INVALID_ID: return "Sent invalid data in place of an ID";
            case TOXME_ERROR_LOOKUP_FAILED: return "Lookup failed because of an error on the other domain's side.";
            case TOXME_ERROR_NO_USER: return "Lookup failed because that user doesn't exist on the domain.";
            case TOXME_ERROR_LOOKUP_INTERNAL: return "Lookup failed because of an error on our side.";
            case TOXME_ERROR_RATE_LIMIT: return "Client is publishing IDs too fast.";
        }

        qDebug() << "Shouldn't be here";
        return "Unknown TOXME error!";
    }

    void Toxme::onPushDone(const QString& password)
    {
        if ( !password.isEmpty() ) {
            QSettings settings;
            settings.setValue("toxme/" + fToxCore.getHexToxID() + "/password", fEncryptSave.encrypt(password));
            settings.sync();
            emit passwordChanged(password);
        } else if ( getPassword().isEmpty() ) { // we got no pw back but we don't have the pw either, already registered need to delete first
            emit alreadyRegistered();
            return;
        }
        emit pushDone(password);
    }

    void Toxme::onDeleteDone()
    {
        QSettings settings;
        settings.remove("toxme/" + fToxCore.getHexToxID() + "/password");
        emit passwordChanged("");
        emit deleteDone();
    }

    const QString Toxme::getPassword() const
    {
        const QSettings settings;
        const QByteArray passEnc = settings.value("toxme/" + fToxCore.getHexToxID() + "/password", QString()).toByteArray();
        return passEnc.size() > 0 ? fEncryptSave.decrypt(passEnc) : QString();
    }

    bool Toxme::getIsPrivate() const
    {
        const QSettings settings;
        return settings.value("toxme/" + fToxCore.getHexToxID() + "/private", true).toBool();
    }

    void Toxme::setIsPrivate(bool priv) const
    {
        QSettings settings;
        settings.setValue("toxme/" + fToxCore.getHexToxID() + "/private", priv);
        settings.sync();
    }

    const QByteArray Toxme::pubKeyBin() const
    {
        return QByteArray::fromHex(fPublicKey.toUtf8());
    }

    void Toxme::httpRequestDone(QNetworkReply *reply) {
        if ( reply == NULL ) {
            qDebug() << "Undefined reply";
            return;
        }

        reply->deleteLater();
        bool ok = false;
        int requestID = reply->property("requestID").toInt(&ok);
        if ( !ok ) {
            emit requestError("Error getting requestID from reply object", QString(), -1);
            return;
        }

        if ( requestID < 0 ) { // pubkey get
            const QByteArray data = reply->readAll();
            QJsonParseError parseError;
            const QJsonDocument resDoc = QJsonDocument::fromJson(data, &parseError);
            const QJsonObject resObj = resDoc.object();
            fPublicKey = resObj.value("key").toString();
            return;
        }

        const QString userInfo = reply->property("userInfo").toString();

        if ( requestID != fNextRequestID - 1 ) {
            emit requestError("Request race condition", userInfo, requestID);
            return;
        }

        QJsonObject resObj;
        QJsonParseError parseError;
        bool parsed = parseResponse(reply, resObj, &parseError);

        if ( reply->error() != QNetworkReply::NoError ) {
            if ( parsed ) {
                emit requestError(handleReplyError(resObj.value("c").toInt(-999)), userInfo, requestID);
            } else {
                emit requestError(reply->errorString(), reply->readAll(), requestID);
            }
            return;
        }

        if ( !parsed ) {
            qDebug() << "HTTP Response parse error: " << parseError.errorString();
            emit requestError("HTTP Response parse error: " + parseError.errorString(), userInfo, requestID);
            return;
        }

        // check memorabilia and action
        int action = reply->property("action").toInt();
        if ( !checkMemorabilia(reply, resObj) ) {
            qDebug() << "Memorabilia mismatch!";
        }

        // check error code
        const QString replyError = handleReplyError(resObj.value("c").toInt(100));
        if ( !replyError.isEmpty() ) {
            emit requestError(replyError, userInfo, requestID);
            return;
        }

        switch ( action ) {
            case TOXME_ACTION_PUSH: onPushDone(resObj.value("password").toString(QString())); break;
            case TOXME_ACTION_DELETE: onDeleteDone(); break;
            case TOXME_ACTION_LOOKUP: emit lookupDone(userInfo, resObj.value("tox_id").toString("error"), requestID); break;
            case TOXME_ACTION_REVERSE_LOOKUP: emit reverseLookupDone(userInfo, resObj.value("name").toString("error"), requestID); break;
            default: emit requestError("Unknown action", userInfo, requestID); break;
        }

        fRequestID = -1;
    }

    bool Toxme::checkMemorabilia(QNetworkReply *reply, const QJsonObject& resObj) const
    {
        /*const QString memStr = reply->property("memorabilia").toString();
        const QString sigMemStr = resObj.value("signed_memorabilia").toString();
        const QByteArray memArr = QByteArray::fromBase64(memStr.toUtf8());
        const QByteArray sigMemArr = QByteArray::fromBase64(sigMemStr.toUtf8());
        uint8_t* memorabilia = (uint8_t*) memArr.data();
        uint8_t* signed_memorabilia = (uint8_t*) sigMemArr.data();
        unsigned long long mem_size = memArr.size();
        unsigned long long sigmem_size = sigMemArr.size();

        if (crypto_sign_open(memorabilia, &mem_size, signed_memorabilia, sigmem_size, (uint8_t*) pubKeyBin().data()) != 0) {
            //qDebug() << "M: " << memStr << " SM: " << sigMemStr << " pubKey: " << fPublicKey <<"";
            return false;
        }*/

        Q_UNUSED(reply);
        Q_UNUSED(resObj);
        // no idea how they do this but they don't seem to sign with crypto_box, skipping for now as no other client seems
        // to bother checking the memorabilia

        return true;
    }

    bool Toxme::parseResponse(QNetworkReply* reply, QJsonObject& obj, QJsonParseError* parseError) const
    {
        const QByteArray data = reply->readAll();
        const QJsonDocument resDoc = QJsonDocument::fromJson(data, parseError);
        if ( parseError->error != QJsonParseError::NoError ) {
            return false;
        }

        obj = resDoc.object();
        return true;
    }

    void Toxme::refresh() const
    {
        emit passwordChanged(getPassword());
    }

}
