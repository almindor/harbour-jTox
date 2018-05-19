#include "event.h"
#include "utils.h"
#include <QLocale>
#include <QDebug>

namespace JTOX {

    Event::Event() : fID(-1), fFriendID(0), fEventType(etEdit), fMessage(QString()), fSendID(0), fFileID(), fFileSize(0), fFilePosition(0), fFilePausers(0)
    {

    }

    Event::Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString& message, qint64 sendID) :
        fID(id), fFriendID(friendID), fEventType(eventType), fMessage(message), fSendID(sendID),
        fFileID(), fFileSize(0), fFilePosition(0), fFilePausers(0)
    {
        createdAt.setTimeSpec(Qt::UTC);
        fCreated = createdAt.toLocalTime();
    }

    Event::Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString &message, qint64 sendID,
                 const QString& filePath, const QByteArray& fileID, quint64 fileSize, quint64 filePosition, int pausers) :
        fID(id), fFriendID(friendID), fEventType(eventType), fMessage(message), fSendID(sendID),
        fFilePath(filePath), fFileID(fileID), fFileSize(fileSize), fFilePosition(filePosition), fFilePausers(pausers)
    {
        createdAt.setTimeSpec(Qt::UTC);
        fCreated = createdAt.toLocalTime();
    }

    const QVariant Event::value(int role) const {
        switch ( role ) {
            case erID: return fID;
            case erCreated: return fCreated;
            case erEventType: return fEventType;
            case erMessage: return hyperLink(fMessage);
            case erFileName: return fMessage; // ensure we don't attempt a hyperlink!
            case erFilePath: return fFilePath;
            case erFileID: return fFileID;
            case erFileSize: return fileSize();
            case erFilePosition: return fFilePosition;
            case erFilePausers: return fFilePausers;
        }

        return QVariant("invalid_role");
    }

    int Event::id() const {
        return fID;
    }

    void Event::setID(int id)
    {
        fID = id;
    }

    void Event::setCreatedAt(const QDateTime &created_at)
    {
        fCreated = created_at;
    }

    const QString Event::message() const
    {
        return fMessage;
    }

    qint64 Event::sendID() const
    {
        return fSendID;
    }

    quint32 Event::friendID() const
    {
        return fFriendID;
    }

    EventType Event::type() const
    {
        return fEventType;
    }

    void Event::setSendID(qint64 sendID)
    {
        fSendID = sendID;
    }

    void Event::setEventType(EventType eventType)
    {
        fEventType = eventType;
    }

    bool Event::isFile() const
    {
        switch ( fEventType ) {
            case etFileTransferIn:
            case etFileTransferInCanceled:
            case etFileTransferInPaused:
            case etFileTransferOut:
            case etFileTransferOutCanceled:
            case etFileTransferOutPaused:
            case etFileTransferInRunning:
            case etFileTransferOutRunning:
            case etFileTransferInDone:
            case etFileTransferOutDone: return true;
            default: return false;
        }
    }

    bool Event::isIncoming() const
    {
        if ( !isFile() ) {
            return false;
        }

        switch ( fEventType ) {
            case etFileTransferIn:
            case etFileTransferInCanceled:
            case etFileTransferInPaused:
            case etFileTransferInRunning:
            case etFileTransferInDone: return true;
            default: return false;
        }
    }

    const QByteArray Event::fileID() const
    {
        return fFileID;
    }

    const QString Event::fileName() const
    {
        return fMessage;
    }

    const QString Event::filePath() const
    {
        return fFilePath;
    }

    quint64 Event::fileSize() const
    {
        return fFileSize;
    }

    quint64 Event::filePosition() const
    {
        return fFilePosition;
    }

    int Event::filePausers() const
    {
        return fFilePausers;
    }

    void Event::setFilePosition(quint64 position)
    {
        fFilePosition = position;
    }

    void Event::setFilePausers(int pausers)
    {
        fFilePausers = pausers;
    }

    const QString Event::hyperLink(const QString& message) const
    {
        int n = message.indexOf("http://");
        if ( n < 0 ) n = message.indexOf("https://");

        if ( n < 0 ) return message;

        if ( n > 0 && !message.at(n - 1).isSpace() ) return message; // probably wrapped already

        QString link = "";
        for ( int i = n; i < message.length(); i++ ) {
            if ( message.at(i).isSpace() ) {
                break;
            }
            link += message.at(i);
        }

        QString newMsg = message;
        return hyperLink(newMsg.replace(link, "<a href=\"" + link + "\">" + link + "</a>").replace('\n', "<br>"));
    }

    void Event::delivered() {
        if ( fEventType != etMessageOutPending ) {
            Utils::bail("Delivered called on non pending message. MessageID: " + QString::number(fID) + " event type: " + QString::number(fEventType));
        }

        fEventType = etMessageOut;
    }

    void Event::viewed()
    {
        if ( fEventType != etMessageInUnread ) {
            Utils::bail("Viewed called on non unread message. MessageID: " + QString::number(fID) + " event type: " + QString::number(fEventType));
        }

        fEventType = etMessageIn;
    }

}
