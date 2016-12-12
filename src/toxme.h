#ifndef TOXME_H
#define TOXME_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonParseError>
#include "toxcore.h"

namespace JTOX {

    class Toxme : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QString domain READ getDomain NOTIFY domainChanged)
        Q_PROPERTY(int requestID MEMBER fRequestID NOTIFY nextRequest)
        Q_PROPERTY(QString password READ getPassword NOTIFY passwordChanged)
        Q_PROPERTY(bool isPrivate READ getIsPrivate WRITE setIsPrivate NOTIFY passwordChanged)
    public:
        Toxme(const ToxCore& toxCore, EncryptSave& encryptSave);
        Q_INVOKABLE int lookup(const QString& name);
        Q_INVOKABLE int reverseLookup(const QString& id);
        Q_INVOKABLE int push(const QString& name, const QString& toxID, const QString& bio, bool priv = true);
        Q_INVOKABLE int remove();
        const QString getDomain() const;
        int requestID() const;
    signals:
        void pushDone(const QString& password) const;
        void deleteDone() const;
        void lookupDone(const QString& name, const QString& toxID, int requestID) const;
        void reverseLookupDone(const QString& publicKey, const QString& name, int requestID) const;
        void requestError(const QString& error, const QString& userInfo, int requestID) const;
        void nextRequest(int requestID) const;
        void passwordChanged(const QString& password) const;
        void domainChanged(const QString& domain) const;
        void alreadyRegistered() const;
    private:
        const ToxCore& fToxCore;
        EncryptSave& fEncryptSave;
        QNetworkAccessManager fNetManager;
        int fRequestID;
        int fNextRequestID;
        QString fPublicKey;

        int publicRequest(QJsonObject& payload, int action, const QString& userInfo);
        int privateRequest(const QJsonObject& encPayload, int action, const QString& userInfo);
        int postRequest(QJsonObject& payload, int action, const QString& userInfo);
        const QString generateMemorabilia() const;
        const QString handleReplyError(int error) const;
        void onPushDone(const QString& password);
        void onDeleteDone();
        const QString getPassword() const;
        bool getIsPrivate() const;
        void setIsPrivate(bool priv) const;
        const QByteArray pubKeyBin() const;
    private slots:
        void httpRequestDone(QNetworkReply* reply);
        bool checkMemorabilia(QNetworkReply* reply, const QJsonObject& resObj) const;
        bool parseResponse(QNetworkReply* reply, QJsonObject& obj, QJsonParseError* parseError) const;
        void refresh() const;
    };

}

#endif // TOXME_H

