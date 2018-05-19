#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QVariant>
#include <QTimer>
#include <QMap>
#include <tox/tox.h>
#include "toxcore.h"
#include "friendmodel.h"
#include "event.h"
#include "encryptsave.h"
#include "dbdata.h"

namespace JTOX {

    class EventModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int friendID READ getFriendID NOTIFY friendUpdated)
        Q_PROPERTY(QString friendName READ getFriendName NOTIFY friendUpdated)
        Q_PROPERTY(int friendStatus READ getFriendStatus NOTIFY friendUpdated)
        Q_PROPERTY(bool friendTyping READ getFriendTyping NOTIFY friendUpdated)
        Q_PROPERTY(bool typing READ getTyping WRITE setTyping NOTIFY typingChanged)
    public:
        EventModel(ToxCore& toxCore, FriendModel& friendModel, DBData& dbData);
        virtual ~EventModel();
        QHash<int, QByteArray> roleNames() const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        int getFriendID() const;
        qint64 sendMessageRaw(const QString& message, qint64 friendID, int id);

        Q_INVOKABLE qint64 setFriendIndex(int friendIndex);
        Q_INVOKABLE void setFriend(qint64 friendID);
        Q_INVOKABLE void sendMessage(const QString& message);
        Q_INVOKABLE void deleteMessage(int eventID);
        Q_INVOKABLE bool fileExists(int eventID);
        Q_INVOKABLE void sendFile(const QString& filePath);
        Q_INVOKABLE bool deleteFile(int eventID);
        Q_INVOKABLE void pauseFile(int eventID);
        Q_INVOKABLE void resumeFile(int eventID);
        Q_INVOKABLE void cancelFile(int eventID);
        Q_INVOKABLE void refreshFilePosition(int index);
    signals:
        void friendUpdated() const;
        void typingChanged(bool typing) const;
        void transferError(const QString& error) const;
        void eventError(const QString& error) const;
        void transferComplete(const QString& fileName, int friendIndex, const QString& friendName);
        void messageReceived(int friendIndex, const QString& friendName) const;
        void transferReceived(int friendIndex, const QString& friendName) const;
    public slots:
        void onMessageDelivered(quint32 friendID, quint32 sendID);
        void onMessageReceived(quint32 friend_id, TOX_MESSAGE_TYPE type, const QString& message);
        void onFriendUpdated(quint32 friend_id);
        void onFriendWentOnline(quint32 friendID);
        void onFileReceived(quint32 friend_id, quint32 file_number, quint64 file_size, const QString& file_name);
        void onFileChunkReceived(quint32 friend_id, quint32 file_number, quint64 position, const QByteArray& data);
        void onFileChunkRequest(quint32 friend_id, quint32 file_number, quint64 position, size_t length);
        void onFileCanceled(quint32 friend_id, quint32 file_number);
        void onFilePaused(quint32 friend_id, quint32 file_number);
        void onFileResumed(quint32 friend_id, quint32 file_number);
    private:
        ToxCore& fToxCore;
        FriendModel& fFriendModel;
        DBData fDBData;
        EventList fList;
        QTimer fTimerViewed;
        QTimer fTimerTyping;
        QSqlDatabase fDB;
        QSqlQuery fSelectQuery;
        QSqlQuery fInsertQuery;
        QSqlQuery fDeliveredUpdateQuery;
        qint64 fFriendID;
        bool fTyping;
        QMap <quint64, QFile*> fTransferFiles;

        int indexForEvent(int eventID) const;
        bool handleSendMessageError(TOX_ERR_FRIEND_SEND_MESSAGE error) const;
        bool handleFileControlError(TOX_ERR_FILE_CONTROL error, bool soft = false) const;
        bool handleFileSendChunkError(TOX_ERR_FILE_SEND_CHUNK error, bool soft = false) const;
        int getFriendStatus() const;
        bool getFriendTyping() const;
        const QString getFriendName() const;
        bool getTyping() const;
        void setTyping(bool typing);
        void setTyping(qint64 friendID, bool typing);
        void cancelTransfer(const Event& transfer);
        void cancelTransfers();
        void completeTransfer(const Event& transfer, quint64 position);
        void updateEventType(const Event& event, EventType eventType, const QVector<int>& roles = QVector<int>(1, erEventType));
        void updateEvent(const Event& event, EventType eventType, quint64 filePosition, int filePausers, const QVector<int>& roles);
        QFile* fileForTransfer(const Event& transfer, QIODevice::OpenModeFlag openMode);
    private slots:
        void onMessagesViewed();
        void onTypingDone();
    };

}

#endif // EVENTMODEL_H
