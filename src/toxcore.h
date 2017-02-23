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

#ifndef JTOX_H
#define JTOX_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QTimer>
#include <tox/tox.h>
#include "encryptsave.h"
#include "dbdata.h"

namespace JTOX {

    class Bootstrapper : public QThread
    {
        Q_OBJECT
    public:
        Bootstrapper();
        void run();
        void start(Tox* tox, const QJsonArray& nodes);
        bool fWorking;
    signals:
        void resultReady(int count);
    private:
        Tox* fTox;
        QJsonArray fNodes;
    };

    class PasswordValidator : public QThread
    {
        Q_OBJECT
    public:
        PasswordValidator(EncryptSave& encryptSave);
        void run();
        void start(const QString& password);
        bool fWorking;
    signals:
        void resultReady(bool valid);
    private:
        EncryptSave& fEncryptSave;
        QString fPassword;
    };

    class ToxInitializer : public QThread
    {
        Q_OBJECT
    public:
        ToxInitializer(EncryptSave& encryptSave);
        void run();
        void start(bool initialUse, const QString& password);
        bool fWorking;
    signals:
        void resultReady(void* tox, const QString& error);
    private:
        EncryptSave& fEncryptSave;
        bool fInitialUse;
        QString fPassword;
        bool handleToxNewError(TOX_ERR_NEW error) const;
    };

    class ToxCore : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool busy READ getBusy NOTIFY busyChanged)
        Q_PROPERTY(quint32 majorVersion READ getMajorVersion NOTIFY versionChanged)
        Q_PROPERTY(quint32 minorVersion READ getMinorVersion NOTIFY versionChanged)
        Q_PROPERTY(quint32 patchVersion READ getPatchVersion NOTIFY versionChanged)
        Q_PROPERTY(QString publicKey READ getHexPublicKey NOTIFY accountChanged)
        Q_PROPERTY(QString toxID READ getHexToxID NOTIFY accountChanged)
        Q_PROPERTY(QString noSpam READ getNoSpam NOTIFY accountChanged)
        Q_PROPERTY(QString userName READ getUserName WRITE setUserName NOTIFY userNameChanged)
        Q_PROPERTY(QString statusMessage READ getStatusMessage WRITE setStatusMessage NOTIFY statusMessageChanged)
        Q_PROPERTY(int status READ getStatus WRITE setStatus NOTIFY statusChanged)
        Q_PROPERTY(QString statusText READ getStatusText NOTIFY statusChanged)
        Q_PROPERTY(bool initialUse READ getInitialUse NOTIFY clientReset)
        Q_PROPERTY(bool passwordValid READ getPasswordValid NOTIFY passwordValidChanged)
        Q_PROPERTY(bool initialized READ getInitialized NOTIFY clientReset)
    public:
        ToxCore(EncryptSave& encryptSave, DBData& dbData);
        virtual ~ToxCore();

        Tox* tox();
        void setConnectionStatus();
        void onFriendRequest(const QString& hexKey, const QString& message);
        void onMessageReceived(quint32 friend_id, TOX_MESSAGE_TYPE type, const QString& message);
        void onMessageDelivered(quint32 friend_id, quint32 message_id);

        void onFriendStatusChanged(quint32 friend_id, int status);
        void onFriendConStatusChanged(quint32 friend_id, int status);
        void onFriendStatusMsgChanged(quint32 friend_id, const QString& statusMessage);
        void onFriendNameChanged(quint32 friend_id, const QString& name);
        void onFriendTypingChanged(quint32 friend_id, bool typing);

        bool getBusy() const;
        bool getInitialized() const;
        bool getInitialUse() const;
        bool getApplicationActive() const;
        const QByteArray encryptPayload(const QByteArray& payload, const QByteArray& pk, const QByteArray& nonce) const;
        const QString getHexPublicKey() const;
        const QString getHexToxID() const;
        void save();

        Q_INVOKABLE void init(const QString& password);
        Q_INVOKABLE bool setNoSpam(const QString& hexVal); // we need to knox if the value is ok
        Q_INVOKABLE void validatePassword(const QString& password);
        Q_INVOKABLE void setApplicationActive(bool active);
        Q_INVOKABLE bool importAccount(const QString& fileName);
        Q_INVOKABLE void exportAccount() const;
        Q_INVOKABLE void wipeLogs(); // used for various debugging reasons
    signals:
        void clientReset();
        void versionChanged();
        void connectionStatusChanged(const QString& status);
        void statusChanged(int status);
        void accountChanged();

        void friendStatusChanged(quint32 friend_id, int status) const;
        void friendConStatusChanged(quint32 friend_id, int status) const;
        void friendStatusMsgChanged(quint32 friend_id, const QString& statusMessage) const;
        void friendNameChanged(quint32 friend_id, const QString& name) const;
        void friendTypingChanged(quint32 friend_id, bool typing) const;

        void friendRequest(const QString& publicKey, const QString& message);
        void statusMessageChanged(const QString& sm);
        void userNameChanged(const QString& sm);
        void busyChanged(bool busy);
        void messageDelivered(quint32 friendID, quint32 messageID);
        void messageReceived(quint32 friendID, TOX_MESSAGE_TYPE type, const QString& message);
        void passwordValidChanged(bool valid);
        void accountExported(const QString& fileName) const;
        void accountImported() const;
        void errorOccurred(const QString& error) const;
        void logsWiped() const;
    private slots:
        void httpRequestDone(QNetworkReply *reply);
        void bootstrappingDone(int count);
        void passwordValidationDone(bool valid);
        void toxInitDone(void* tox, const QString& error);
        void iterate();
    private:
        EncryptSave& fEncryptSave;
        DBData& fDBData;
        Tox* fTox;
        Bootstrapper fBootstrapper;
        ToxInitializer fInitializer;
        PasswordValidator fPasswordValidator;
        QNetworkAccessManager fNetManager;
        QNetworkReply* fNodesRequest;
        QTimer fTimer;
        bool fPasswordValid;
        bool fInitialized;
        bool fApplicationActive;

        quint32 getMajorVersion() const;
        quint32 getMinorVersion() const;
        quint32 getPatchVersion() const;
        bool getPasswordValid() const;
        const QString getNoSpam() const;
        int getStatus() const;
        const QString getStatusText() const;
        void setStatus(int status);
        const QString getStatusMessage() const;
        void setStatusMessage(const QString& sm);
        const QString getUserName() const;
        void setUserName(const QString& uname);
        bool getKeepLogs() const;
        void setKeepLogs(bool keep);
        void killTox();
    };

}

#endif // JTOX_H
