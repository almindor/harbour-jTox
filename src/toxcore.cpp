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

#include <sailfishapp.h>
#include "toxcore.h"
#include "utils.h"
#include "c_callbacks.h"
#include <sodium/utils.h>
#include <sodium/crypto_box.h>
#include <sodium/randombytes.h>
#include <QtDebug>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QSettings>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

namespace JTOX {

    //****************************Bootstrapper****************************//

    Bootstrapper::Bootstrapper() : QThread(0)
    {
        fWorking = false;
    }

    void Bootstrapper::run() {
        fWorking = true;
        int count = bootstrapNodes(4);

        fWorking = false;
        emit resultReady(count);
    }

    void Bootstrapper::start(Tox* tox, const QJsonArray &nodes) {
        fTox = tox;
        fNodes = nodes;
        QThread::start();
    }

    int Bootstrapper::bootstrapNodes(int maxNodes)
    {
        QJsonArray nodes;
        // we only consider those nodes that have both TCP and UDP enabled
        foreach ( const QJsonValue& nodeVal, fNodes ) {
            const QJsonObject node = nodeVal.toObject(); // pick a random node from the list
            bool status_udp = node.value("status_udp").toBool();
            bool status_tcp = node.value("status_tcp").toBool();
            if ( status_udp && status_tcp ) {
                nodes.append(node);
            }
        }

        TOX_ERR_BOOTSTRAP error = TOX_ERR_BOOTSTRAP_NULL;
        QMap<QString, bool> addedNodes;
        bool ok = false;

        for ( int i = 0; i < 100; i++ ) { // max tries since random can give us same index in a row
            int random_n = (int) randombytes_uniform(nodes.size());
            const QJsonObject node = nodes.at(random_n).toObject(); // pick a random node from the list
            const QString hexKey = node.value("public_key").toString();
            if ( addedNodes.contains(hexKey) ) continue; // already got this one

            const QString ipv4 = node.value("ipv4").toString();
            const QString ipv6 = node.value("ipv6").toString();
            const QString address = ipv6.length() > 1 ? ipv6 : ipv4; // prefer ipv6
            const uint16_t port = (uint16_t) node.value("port").toInt();
            uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
            Utils::hex_to_key(hexKey, public_key);

            ok = tox_bootstrap(fTox, address.toUtf8().data(), port, public_key, &error);
            if ( !ok || error != TOX_ERR_BOOTSTRAP_OK ) {
                continue;
            }

            // this shouldn't be needed as tox_bootstrap should do the relay as well
            /*ok = tox_add_tcp_relay(fTox, address.toUtf8().data(), port, public_key, &error);
            if ( !ok || error != TOX_ERR_BOOTSTRAP_OK ) {
                continue;
            }*/

            addedNodes[hexKey] = true;
            if ( addedNodes.size() >= maxNodes ) {
                break; // we found our max nodes count, done
            }
        }

        return addedNodes.size();
    }

    //******************************PasswordValidator*********************//

    PasswordValidator::PasswordValidator(EncryptSave& encryptSave) : QThread(0), fEncryptSave(encryptSave)
    {
        fWorking = false;
    }

    void PasswordValidator::run()
    {
        fWorking = true;
        const QSettings settings;
        const QByteArray encryptedData = settings.value("tox/savedata", QByteArray()).toByteArray();

        fEncryptSave.setPassword(fPassword, encryptedData);
        fPassword = QString(); // wipe password from this instance
        // password is valid if it can decrypt or if the data is not encrypted
        bool result = fEncryptSave.isEncrypted(encryptedData) ? fEncryptSave.validateDecrypt(encryptedData) : true;
        fWorking = false;
        emit resultReady(result);
    }

    void PasswordValidator::start(const QString& password)
    {
        fPassword = password;
        QThread::start();
    }

    //****************************ToxInitializer***************************//

    ToxInitializer::ToxInitializer(EncryptSave& encryptSave) : QThread(0), fEncryptSave(encryptSave)
    {
        fWorking = false;
    }

    void ToxInitializer::run()
    {
        fWorking = true;
        QSettings settings;
        TOX_ERR_NEW error;
        QByteArray saveData; // must outlive options
        struct Tox_Options options;
        tox_options_default(&options);

        if ( !fInitialUse ) {
            const QByteArray encryptedData = settings.value("tox/savedata", QByteArray()).toByteArray();

            // if out profile is not encrypted we just set the password and it gets saved with it right after init
            if ( !fEncryptSave.isEncrypted(encryptedData) ) {
                fEncryptSave.setPassword(fPassword);
                saveData = encryptedData;
            } else { // otherwise set password with salt from encrypted data
                fEncryptSave.setPassword(fPassword, encryptedData);
                saveData = fEncryptSave.decryptRaw(encryptedData);
            }

            options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
            options.savedata_data = (uint8_t*) saveData.data();
            options.savedata_length = saveData.size();
        } else {
            fEncryptSave.setPassword(fPassword); // set password with random salt for new account
        }

        Tox* tox = tox_new(&options, &error);
        if ( !handleToxNewError(error) ) {
            fWorking = false;
            emit resultReady(NULL, "Error creating tox service object");
            return;
        }

        if ( fInitialUse ) {
            const QByteArray status_message = tr("Hi from jTox on Sailfish").toUtf8();
            tox_self_set_status_message(tox, (uint8_t*) status_message.data(), strnlen(status_message, 100), NULL);
        }

        // register callbacks
        tox_callback_self_connection_status(tox, c_connection_status_cb);
        tox_callback_friend_request(tox, c_friend_request_cb);
        tox_callback_friend_message(tox, c_friend_message_cb);
        tox_callback_friend_read_receipt(tox, c_friend_read_receipt_cb);
        tox_callback_friend_connection_status(tox, c_friend_connection_status_cb);
        tox_callback_friend_name(tox, c_friend_name_cb);
        tox_callback_friend_status(tox, c_friend_status_cb);
        tox_callback_friend_status_message(tox, c_friend_status_message_cb);
        tox_callback_friend_typing(tox, c_friend_typing_cb);
        tox_callback_file_recv(tox, c_tox_file_recv_cb);
        tox_callback_file_recv_chunk(tox, c_tox_file_recv_chunk_cb);

        fWorking = false;
        emit resultReady(tox, QString());
    }

    void ToxInitializer::start(bool initialUse, const QString& password)
    {
        fInitialUse = initialUse;
        fPassword = password;
        QThread::start();
    }

    bool ToxInitializer::handleToxNewError(TOX_ERR_NEW error) const
    {
        switch ( error ) {
            case TOX_ERR_NEW_OK: return true;
            case TOX_ERR_NEW_NULL: Utils::bail("Cannot create toxcore, null argument");
            case TOX_ERR_NEW_MALLOC: Utils::bail("Cannot create toxcore, malloc error");
            case TOX_ERR_NEW_PORT_ALLOC: Utils::bail("Cannot create toxcore, port bind permission error");
            case TOX_ERR_NEW_PROXY_BAD_TYPE: Utils::bail("Cannot create toxcore, invalid proxy type");
            case TOX_ERR_NEW_PROXY_BAD_HOST: Utils::bail("Cannot create toxcore, bad proxy host");
            case TOX_ERR_NEW_PROXY_BAD_PORT: Utils::bail("Cannot create toxcore, bad proxy port");
            case TOX_ERR_NEW_PROXY_NOT_FOUND: Utils::bail("Cannot create toxcore, proxy not found");
            case TOX_ERR_NEW_LOAD_ENCRYPTED: Utils::bail("Cannot create toxcore, save data encrypted");
            case TOX_ERR_NEW_LOAD_BAD_FORMAT: Utils::bail("Cannot create toxcore, save data invalid");
        }

        return false;
    }


    //*******************************ToxCore******************************//

    const int ACTIVE_ITERATION_DELAY = 250;
    const int PASSIVE_ITERATION_DELAY = 2000;
    const int AWAY_DELAY = 300000; // 5m for away

    ToxCore::ToxCore(EncryptSave& encryptSave, DBData& dbData) : QObject(0),
        fEncryptSave(encryptSave), fDBData(dbData),
        fTox(NULL), fBootstrapper(), fInitializer(encryptSave), fPasswordValidator(encryptSave),
        fNodesRequest(NULL), fIterationTimer(), fPasswordValid(false), fInitialized(false)
    {
        connect(&fNetManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpRequestDone(QNetworkReply*)));
        connect(&fBootstrapper, &Bootstrapper::resultReady, this, &ToxCore::bootstrappingDone);
        connect(&fInitializer, &ToxInitializer::resultReady, this, &ToxCore::toxInitDone);
        connect(&fPasswordValidator, &PasswordValidator::resultReady, this, &ToxCore::passwordValidationDone);
        connect(&fIterationTimer, &QTimer::timeout, this, &ToxCore::iterate);
        connect(&fAwayTimer, &QTimer::timeout, this, &ToxCore::awayTimeout);

        fIterationTimer.setInterval(ACTIVE_ITERATION_DELAY);
        fAwayTimer.setInterval(AWAY_DELAY);
        fAwayStatus = 0; // offline

        // If we're running first time (or updated from 1.0.1-) we need to "store"
        // the nodes from our defaults
        QSettings settings;
        if ( !settings.contains("tox/nodes") ) {
            settings.setValue("tox/nodes", getDefaultNodes());
        }
    }

    ToxCore::~ToxCore() {
        if ( fInitialized ) {
            awayRestore(); // restore away status so we don't override if killed while in bg mode
            save();
            killTox();
        }
    }

    Tox* ToxCore::tox() {
        if ( fTox == NULL ) {
            qDebug() << "Tox instance not initialized yet\n";
            emit errorOccurred("Tox instance not initialized yet");
        }

        return fTox;
    }

    void ToxCore::setConnectionStatus() {
        if ( getStatus() > 0 ) {
            awayStart(); // if we went back online but we're minimized start away timer
        }
        emit accountChanged();
        emit statusChanged(getStatus());
    }

    void ToxCore::onFriendRequest(const QString& hexKey, const QString& message) {
        emit friendRequest(hexKey, message);
    }

    void ToxCore::onMessageReceived(quint32 friend_id, TOX_MESSAGE_TYPE type, const QString& message) {
        emit messageReceived(friend_id, type, message);
    }

    void ToxCore::onMessageDelivered(quint32 friend_id, quint32 message_id) {
        emit messageDelivered(friend_id, message_id);
    }

    void ToxCore::onFriendStatusChanged(quint32 friend_id, int status)
    {
        save();
        emit friendStatusChanged(friend_id, status);
    }

    void ToxCore::onFriendConStatusChanged(quint32 friend_id, int status)
    {
        save();
        emit friendConStatusChanged(friend_id, status);
    }

    void ToxCore::onFriendStatusMsgChanged(quint32 friend_id, const QString& statusMessage)
    {
        save();
        emit friendStatusMsgChanged(friend_id, statusMessage);
    }

    void ToxCore::onFriendNameChanged(quint32 friend_id, const QString& name)
    {
        save();
        emit friendNameChanged(friend_id, name);
    }

    void ToxCore::onFriendTypingChanged(quint32 friend_id, bool typing)
    {
        emit friendTypingChanged(friend_id, typing);
    }

    void ToxCore::onFileReceived(quint32 friend_id, quint32 file_id, quint64 file_size, const QString &file_name) const
    {
        emit fileReceived(friend_id, file_id, file_size, file_name);
    }

    void ToxCore::onFileChunkReceived(quint32 friend_id, quint32 file_id, quint64 position, const quint8 *data, size_t length) const
    {
        emit fileChunkReceived(friend_id, file_id, position, data, length);
    }

    bool ToxCore::getBusy() const {
        /*qDebug() << "Nodes running: " << ( fNodesRequest != NULL && fNodesRequest->isRunning() ) << "\n";
        qDebug() << "fBootstrapper running: " << fBootstrapper.fWorking << "\n";
        qDebug() << "fPasswordValidator running: " << fPasswordValidator.fWorking << "\n";
        qDebug() << "fInitializer running: " << fInitializer.fWorking << "\n";*/

        return (( fNodesRequest != NULL && fNodesRequest->isRunning() ) ||
                  fBootstrapper.fWorking ||
                  fPasswordValidator.fWorking ||
                fInitializer.fWorking );
    }

    bool ToxCore::getInitialized() const
    {
        return fInitialized;
    }

    bool ToxCore::getInitialUse() const {
        const QSettings settings;
        return !settings.contains("tox/savedata");
    }

    bool ToxCore::getApplicationActive() const
    {
        return fApplicationActive;
    }

    const QByteArray ToxCore::encryptPayload(const QByteArray &payload, const QByteArray& pk, const QByteArray& nonce) const {
        if ( !fInitialized ) {
            Utils::bail("Attempting to retreive secret key on unintialized tox");
        }
        uint8_t sk[TOX_SECRET_KEY_SIZE];
        tox_self_get_secret_key(fTox, sk);

        const size_t cypherlen = crypto_box_MACBYTES + payload.size();
        QByteArray payloadEnc(cypherlen, 0);

        int cryptResult = crypto_box_easy((uint8_t*) payloadEnc.data(), (uint8_t*) payload.data(), payload.size(),
                        (uint8_t*) nonce.data(), (uint8_t*) pk.constData(), sk);

        if ( cryptResult != 0 ) {
            Utils::bail("Error on payload encryption");
        }

        return payloadEnc.toBase64();
    }

    void ToxCore::init(const QString& password)
    {
        if ( fInitializer.isRunning() ) {
            emit errorOccurred("Busy on init");
            return;
        }

        if ( fInitialized ) {
            killTox();
        }
        emit busyChanged(true);
        fInitializer.start(getInitialUse(), password);
    }

    void ToxCore::iterate() {
        if ( !fInitialized ) {
            return;
        }
        tox_iterate(fTox, this);
    }

    void ToxCore::awayTimeout()
    {
        // set us to away if we're connected and not away yet
        int status = getStatus();
        if ( status > 0 && status != 2 ) {
            setStatus(2);
        }
    }

    quint32 ToxCore::getMajorVersion() const {
        return tox_version_major();
    }

    quint32 ToxCore::getMinorVersion() const {
        return tox_version_minor();
    }

    quint32 ToxCore::getPatchVersion() const {
        return tox_version_patch();
    }

    bool ToxCore::getPasswordValid() const
    {
        if ( getInitialUse() ) {
            return true;
        }

        return fPasswordValid;
    }

    const QString ToxCore::getHexPublicKey() const {
        if ( !fInitialized ) {
            Utils::bail("Attempt to get public key with uninitialized tox");
        }

        uint8_t pubRaw[TOX_PUBLIC_KEY_SIZE];
        tox_self_get_public_key(fTox, pubRaw);
        return Utils::key_to_hex(pubRaw, TOX_PUBLIC_KEY_SIZE);
    }

    const QString ToxCore::getHexToxID() const
    {
        if ( !fInitialized ) {
            Utils::bail("Attempt to get public toxID with uninitialized tox");
        }

        uint8_t addrRaw[TOX_ADDRESS_SIZE];
        tox_self_get_address(fTox, addrRaw);
        return Utils::key_to_hex(addrRaw, TOX_ADDRESS_SIZE);
    }

    const QString ToxCore::getNoSpam() const
    {
        if ( !fInitialized ) {
            Utils::bail("Attempt to get nospam with uninitialized tox");
        }

        uint32_t noSpamInt = tox_self_get_nospam(fTox);
        return QString::number(noSpamInt, 16).toUpper();
    }

    bool ToxCore::setNoSpam(const QString& hexVal) {
        if ( !fInitialized ) {
            Utils::bail("Attempting to set nospam on unintialized tox");
        }

        bool ok = false;
        uint32_t noSpamInt = hexVal.toUInt(&ok, 16);

        if ( !ok ) {
            return false;
        }

        tox_self_set_nospam(fTox, noSpamInt);
        save();
        emit accountChanged();
        return true;
    }

    void ToxCore::validatePassword(const QString &password)
    {
        fPasswordValid = false;
        if ( getInitialUse() ) {
            fPasswordValid = password.length() >= 8; // if we have nothing encrypted the pw is ok as long as 8chars+
            emit passwordValidChanged(fPasswordValid);
            return;
        }

        if ( fPasswordValidator.isRunning() ) {
            emit errorOccurred("Busy on validate password");
            return;
        }

        emit busyChanged(true);
        fPasswordValidator.start(password);
    }

    void ToxCore::setApplicationActive(bool active)
    {
        fApplicationActive = active;
        fIterationTimer.setInterval(active ? ACTIVE_ITERATION_DELAY : PASSIVE_ITERATION_DELAY);

        if ( active ) {
            awayRestore(); // if we restored, stop away timer and restore old status
        } else {
            awayStart(); // if we minimized, start away timer
        }
    }

    void ToxCore::newAccount()
    {
        QSettings settings;
        settings.remove("tox/savedata");
        settings.sync();
        fDBData.wipe(-1);

        if ( fInitialized ) {
            killTox();
        }

        emit initialUseChanged(true);
        emit accountCreated();
    }

    bool ToxCore::importAccount(const QString& fileName)
    {
        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        QFile impFile(dir.absoluteFilePath(fileName));

        if ( !impFile.open(QIODevice::ReadOnly) ) {
            emit errorOccurred("Import error: " + impFile.errorString());
            return false;
        }

        const QByteArray encryptedData = impFile.readAll();
        impFile.close();

        QSettings settings;
        settings.setValue("tox/savedata", encryptedData);
        settings.sync();

        fIterationTimer.stop();
        fDBData.wipe(-1); // wipe logs without emit
        if ( fInitialized ) {
            killTox();
        }

        emit accountImported();
        return true;
    }

    void ToxCore::exportAccount() const
    {
        const QSettings settings;
        const QByteArray encryptedData = settings.value("tox/savedata", QByteArray()).toByteArray();
        const QDir dir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        const QString userName = getUserName();
        const QString fileName = userName.isEmpty() ? "jtox.tox" : (userName + ".tox");
        QFile expFile(dir.absoluteFilePath(fileName));

        if ( !expFile.open(QIODevice::WriteOnly) ) {
            emit errorOccurred("Export error: " + expFile.errorString());
            return;
        }

        if ( expFile.write(encryptedData) == encryptedData.size() ) {
            emit accountExported(fileName);
        } else {
            emit errorOccurred("Export error: " + expFile.errorString());
        }
        expFile.close();
    }

    void ToxCore::wipeLogs()
    {
        fDBData.wipeLogs();
        emit logsWiped();
    }

    int ToxCore::getStatus() const {
        if ( !fInitialized ) {
            return 0;
        }

        TOX_USER_STATUS userStatus = tox_self_get_status(fTox);
        TOX_CONNECTION conStatus = tox_self_get_connection_status(fTox);
        return Utils::get_overall_status(conStatus, userStatus);
    }

    const QString ToxCore::getStatusText() const
    {
        switch ( getStatus() ) {
            case 0: return tr("Offline");
            case 1: return tr("Ready");
            case 2: return tr("Away");
            case 3: return tr("Busy");
        }

        return tr("Unknown");
    }

    void ToxCore::setStatus(int status) {
        if ( !fInitialized ) {
            Utils::bail("Attempting to set status on unintialized tox");
        }

        TOX_USER_STATUS userStatus = TOX_USER_STATUS_NONE;
        switch ( status ) {
            case 1: userStatus = TOX_USER_STATUS_NONE; break;
            case 2: userStatus = TOX_USER_STATUS_AWAY; break;
            case 3: userStatus = TOX_USER_STATUS_BUSY; break;
            default: Utils::bail("Invalid status update"); break;
        }

        tox_self_set_status(fTox, userStatus);
        save();
        emit statusChanged(status);

        // if we changed status from cover restart away timer, unless set to away already
        if ( status != 2 ) {
            awayStart();
        }
    }

    const QString ToxCore::getStatusMessage() const {
        if ( !fInitialized ) {
            Utils::bail("Attempting to get status msg on unintialized tox");
        }

        size_t size = tox_self_get_status_message_size(fTox);
        uint8_t rawMessage[size];

        tox_self_get_status_message(fTox, rawMessage);
        return QString::fromUtf8((char*)rawMessage, size);
    }

    void ToxCore::setStatusMessage(const QString& sm) {
        if ( !fInitialized ) {
            Utils::bail("Attempting to set status msg on unintialized tox");
        }

        TOX_ERR_SET_INFO error;
        QByteArray rawMessage = sm.toUtf8();
        size_t size = rawMessage.size();
        tox_self_set_status_message(fTox, (uint8_t*)rawMessage.data(), size, &error);

        if ( error != TOX_ERR_SET_INFO_OK ) {
            Utils::bail("Error setting status message: " + QString::number(error, 10));
        }

        save();
        emit statusMessageChanged(sm);
    }

    const QString ToxCore::getUserName() const {
        if ( !fInitialized ) {
            Utils::bail("Attempt to get username with uninitialized tox");
        }

        size_t size = tox_self_get_name_size(fTox);
        uint8_t rawName[size];

        tox_self_get_name(fTox, rawName);
        return QString::fromUtf8((char*)rawName, size);
    }

    void ToxCore::setUserName(const QString& uname) {
        if ( !fInitialized ) {
            Utils::bail("Attempting to set username on unintialized tox");
        }

        TOX_ERR_SET_INFO error;
        QByteArray rawName = uname.toUtf8();
        size_t size = rawName.size();
        tox_self_set_name(fTox, (uint8_t*)rawName.data(), size, &error);

        if ( error != TOX_ERR_SET_INFO_OK ) {
            Utils::bail("Error setting user name: " + QString::number(error, 10));
        }

        save();
        emit userNameChanged(uname);
    }

    const QByteArray ToxCore::getDefaultNodes() const
    {
        const QUrl path = SailfishApp::pathTo("nodes/nodes.json");
        QFile file(path.toLocalFile());
        if ( !file.open(QFile::ReadOnly) ) {
            Utils::bail("Error opening default nodes file");
        }

        return file.readAll();
    }

    void ToxCore::awayRestore()
    {
        fAwayTimer.stop();
        if ( fAwayStatus > 0 ) {
            setStatus(fAwayStatus);
        }
    }

    void ToxCore::awayStart()
    {
        if ( fApplicationActive ) {
            return;
        }

        fAwayStatus = getStatus();
        if ( fAwayStatus > 0 && fAwayStatus != 2 ) { // start timer only if we're not offline or away already
            fAwayTimer.start();
        }
    }

    void ToxCore::save()
    {
        if ( !fInitialized ) {
            Utils::bail("Attempting to save on unintialized tox");
        }

        size_t size = tox_get_savedata_size(fTox);
        const QByteArray saveData(size, 0);
        tox_get_savedata(fTox, (uint8_t*)saveData.data());
        const QByteArray encryptedData = fEncryptSave.encryptRaw(saveData);

        QSettings settings;
        settings.setValue("tox/savedata", encryptedData);
        settings.sync();
        emit initialUseChanged(false);
    }

    void ToxCore::killTox()
    {
        if ( !fInitialized ) {
            Utils::bail("Killtox called when not initialized");
        }

        fInitialized = false;
        tox_kill(fTox);
        fTox = NULL;
    }

    void ToxCore::httpRequestDone(QNetworkReply *reply) {       
        if ( reply == NULL ) {
            emit errorOccurred("Undefined reply");
            return;
        }

        const QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument::fromJson(data, &parseError);

        if ( parseError.error != QJsonParseError::NoError ) {
            emit errorOccurred("HTTP Response parse error: " + parseError.errorString());
            return;
        }

        qint64 currentSeconds = QDateTime::currentMSecsSinceEpoch() / 1000;
        QSettings settings;
        settings.setValue("tox/nodes", data);
        settings.setValue("app/lastnodesrequest", currentSeconds);
        fNodesRequest = NULL;
    }

    void ToxCore::bootstrappingDone(int count) {
        if ( count == 0 ) {
            emit errorOccurred("Bootstrap failed");
        }

        emit busyChanged(false);
    }

    void ToxCore::passwordValidationDone(bool valid)
    {
        fPasswordValid = valid;
        emit busyChanged(false);
        emit passwordValidChanged(fPasswordValid);
    }

    void ToxCore::toxInitDone(void* tox, const QString& error)
    {
        if ( !error.isEmpty() ) {
            emit errorOccurred(error);
            return;
        }

        fTox = (Tox*) tox;
        fInitialized = true;

        const QSettings settings;
        QJsonParseError parseError;
        const QJsonDocument nodesDoc = QJsonDocument::fromJson(settings.value("tox/nodes", "invalid").toByteArray(), &parseError);
        if ( parseError.error != QJsonParseError::NoError ) {
            Utils::bail("Nodes parse error: " + parseError.errorString());
        }
        const QJsonObject nodesObject = nodesDoc.object();
        const QJsonArray nodes = nodesObject.value("nodes").toArray();
        fBootstrapper.start(fTox, nodes);

        // we check for new json once a week
        bool ok = false;
        const QVariant lnr = settings.value("app/lastnodesrequest", 0);
        qint64 lastRequest = lnr.toLongLong(&ok);
        if ( !ok ) {
            Utils::bail("Invalid last nodes request time value: " + lnr.toString());
        }
        qint64 currentSeconds = QDateTime::currentMSecsSinceEpoch() / 1000;
        if ( currentSeconds - lastRequest > 604800 ) {
            QNetworkRequest request(QUrl("https://nodes.tox.chat/json"));
            fNodesRequest = fNetManager.get(request);
        }

        save();
        fIterationTimer.start();
        emit busyChanged(false);
        emit clientReset();
        emit accountChanged();
    }

}
