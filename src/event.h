#ifndef EVENT_H
#define EVENT_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QList>
#include "encryptsave.h"

namespace JTOX {

    enum EventType {
        etEdit = 0,
        etMessageOut = 1,
        etMessageInUnread = 2,
        etMessageOutPending = 3,
        etMessageIn = 4,
        etMessageOutOffline = 5, // pending to be sent
        etCallOutRejected = 6,
        etCallOutAccepted = 7,
        etCallInRejected = 8,
        etCallInAccepted = 9,
        etFileTransferIn = 10,
        etFileTransferOut = 11,
        etFileTransferInPaused = 12,
        etFileTransferOutPaused = 13,
        etFileTransferInCanceled = 14,
        etFileTransferOutCanceled = 15,
        etFileTransferInRunning = 16,
        etFileTransferOutRunning = 17,
        etFileTransferInDone = 18,
        etFileTransferOutDone = 19,
        etCallInPending = 20,
        etCallOutPending = 21,
        etCallInFinished = 22,
        etCallOutFinished = 23
    };

    enum EventRole
    {
        erID = Qt::UserRole + 1,
        erEventType,
        erCreated,
        erMessage,
        erFileName,
        erFilePath,
        erFileID,
        erFileSize,
        erFilePosition,
        erFilePausers,
        erDuration
    };

    class Event
    {
    public:
        Event();
        Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString& message, qint64 sendID);
        Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString& message, qint64 sendID,
              const QString& file_path, const QByteArray& file_id, quint64 fileSize, quint64 filePosition, int pausers);
        const QVariant value(int role) const;
        void delivered();
        void viewed();
        int id() const;
        void setID(int id);
        void setCreatedAt(const QDateTime& created_at);
        const QDateTime createdAt() const;
        const QString message() const;
        qint64 sendID() const;
        quint32 friendID() const;
        EventType type() const;
        void setSendID(qint64 sendID);
        void setEventType(EventType eventType);
        bool isFile() const;
        bool isIncoming() const;
        const QByteArray fileID() const;
        const QString fileName() const; // just an alias
        const QString filePath() const;
        quint64 fileSize() const;
        quint64 filePosition() const;
        int filePausers() const;
        void setFilePosition(quint64 position);
        void setFilePausers(int pausers);
    private:
        int fID;
        quint32 fFriendID;
        EventType fEventType;
        QString fMessage;
        qint64 fSendID;
        QDateTime fCreated;
        QString fFilePath;
        QByteArray fFileID;
        quint64 fFileSize;
        quint64 fFilePosition;
        int fFilePausers;

        const QString hyperLink(const QString& message) const;
    };

    typedef QList<Event> EventList;

}

#endif // EVENT_H
