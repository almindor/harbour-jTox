#ifndef AVATARPROVIDER_H
#define AVATARPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include <QPixmap>
#include "toxcore.h"
#include "dbdata.h"

namespace JTOX {

    // avatar wrapper
    class Avatar {
    public:
        bool equals(const QByteArray& fileID) const;
        bool isEmpty() const;
        void init(const QByteArray& fileID);
        bool initialized() const;
        void addData(const QByteArray& data);
        const QByteArray& data() const;
    private:
        QByteArray fFileID;
        QByteArray fData;
    };

    class AvatarProvider : public QThread, public QQuickImageProvider
    {
        Q_OBJECT
    public:
        AvatarProvider(ToxCore& toxCore, DBData& dbData);
        QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
        const QByteArray getProfileAvatarData() const;
        Q_INVOKABLE void clearAvatar();
        Q_INVOKABLE void setAvatar(const QString& filePath);
        void run() override;
    signals:
        void profileAvatarChanged(const QByteArray& hash, const QByteArray& data) const;
        void avatarChanged(quint32 friend_id) const;
    private slots:
        void onAvatarFileReceived(quint32 friend_id, quint32 file_number, quint64 file_size, const QByteArray &fileID);
        void onFileChunkReceived(quint32 friend_id, quint32 file_number, quint64 position, const QByteArray &data);
    private:
        ToxCore& fToxCore;
        DBData& fDBData;
        QMap <quint64, Avatar> fAvatars; // in memory storage
        QString fAvatarFilePath;
    };

}

#endif // AVATARPROVIDER_H
