#include "event.h"
#include "utils.h"
#include <QLocale>
#include <QDebug>

namespace JTOX {

    Event::Event() : fID(-1), fFriendID(0), fEventType(etEdit), fMessage(QString()), fSendID(0)
    {

    }

    Event::Event(int id, quint32 friendID, QDateTime createdAt, EventType eventType, const QString& message, qint64 sendID) :
        fID(id), fFriendID(friendID), fEventType(eventType), fMessage(message), fSendID(sendID)
    {
        createdAt.setTimeSpec(Qt::UTC);
        fCreated = createdAt.toLocalTime();
    }

    const QVariant Event::value(int role) const {
        switch ( role ) {
            case erID: return fID;
            case erCreated: return fCreated;
            case erEventType: return fEventType;
            case erMessage: return wrapMessage();
            case erFileSize: return fileSize();
            case erFileName: return fileName();
        }

        return QVariant("invalid_role");
    }

    int Event::id() const {
        return fID;
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

    quint64 Event::fileSize() const
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
            case etFileTransferOutDone: {
                quint64 file_size;
                QString file_name;
                Utils::parseFileInfo(fMessage, file_size, file_name);
                return file_size;
            }
            default: return 0;
        }
    }

    const QString Event::fileName() const
    {
        if ( isFile() ) {
            quint64 file_size;
            QString file_name;
            Utils::parseFileInfo(fMessage, file_size, file_name);
            return file_name;
        }

        return QString();
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
        return hyperLink(newMsg.replace(link, "<a href=\"" + link + "\">" + link + "</a>"));
    }

    const QString Event::fileInfo(const QString &message) const
    {
        quint64 file_size;
        QString file_name;
        Utils::parseFileInfo(message, file_size, file_name);
        return file_name;
    }

    const QString Event::wrapMessage() const
    {
        switch ( fEventType ) {
            case etMessageIn:
            case etMessageInUnread:
            case etMessageOut:
            case etMessageOutOffline:
            case etMessageOutPending: return hyperLink(fMessage);
            case etFileTransferIn:
            case etFileTransferInPaused:
            case etFileTransferInCanceled:
            case etFileTransferOut:
            case etFileTransferOutPaused:
            case etFileTransferOutCanceled:
            case etFileTransferInRunning:
            case etFileTransferOutRunning:
            case etFileTransferInDone:
            case etFileTransferOutDone: return fileInfo(fMessage);
            // TODO: handle others
            default: Utils::bail("Invalid event type");
        }

        return QString();
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
