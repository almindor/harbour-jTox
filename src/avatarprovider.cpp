#include "avatarprovider.h"
#include "utils.h"
#include <QBuffer>

namespace JTOX {

    //------------------Avatar-----------------//

    bool Avatar::equals(const QByteArray &fileID) const
    {
        return fFileID == fileID;
    }

    bool Avatar::isEmpty() const
    {
        return fData.isEmpty();
    }

    void Avatar::init(const QByteArray &fileID)
    {
        fData.clear();
        fFileID = fileID;
    }

    bool Avatar::initialized() const
    {
        return !fFileID.isEmpty();
    }

    void Avatar::addData(const QByteArray &data)
    {
        fData += data;
    }

    const QByteArray& Avatar::data() const
    {
        return fData;
    }

    //--------------AvatarProvider-------------//

    AvatarProvider::AvatarProvider(ToxCore& toxCore, DBData& dbData) : QQuickImageProvider(QQuickImageProvider::Pixmap),
        fToxCore(toxCore), fDBData(dbData)
    {
        connect(&toxCore, &ToxCore::avatarFileReceived, this, &AvatarProvider::onAvatarFileReceived);
        connect(&toxCore, &ToxCore::fileChunkReceived, this, &AvatarProvider::onFileChunkReceived);
    }

    QPixmap AvatarProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
    {
        Q_UNUSED(requestedSize);
        Q_UNUSED(size);

        QPixmap pixmap;

        const QStringList ids = id.split('?');

        bool ok = false;
        qint64 friend_id = ids[0].toInt(&ok, 10);
        if ( !ok ) {
            Utils::bail("Unable to cast friend_id::int for avatar");
            return pixmap;
        }

        // try to get from DB if there
        QByteArray data;
        if ( !fDBData.getAvatar(friend_id, data) ) {
            QPixmap empty(1, 1); // TODO: generate image from friend_id
            empty.fill(Qt::transparent);
            return empty;
        }
        pixmap.loadFromData(data);

        return pixmap;
    }

    const QByteArray AvatarProvider::getProfileAvatarData() const
    {
        QByteArray data;
        fDBData.getAvatar(-1, data);

        return data; // empty is fine
    }

    void AvatarProvider::clearAvatar()
    {
        fDBData.clearAvatar(-1);
        emit profileAvatarChanged(QByteArray(), QByteArray());
    }

    void AvatarProvider::setAvatar(const QString& filePath)
    {
        fAvatarFilePath = filePath;
        start();
    }

    void AvatarProvider::run()
    {
        QFile file(fAvatarFilePath);
        if ( !file.open(QIODevice::ReadOnly) ) {
            Utils::bail("Unable to open avatar file", true);
            return;
        }

        const QByteArray pixmapData = file.readAll();
        file.close();

        QPixmap pixmap;
        if ( !pixmap.loadFromData(pixmapData) ) {
            Utils::bail("Unable to load avatar file into image", true);
            return;
        }
        if ( pixmap.width() >= pixmap.height() ) {
            pixmap = pixmap.scaledToWidth(128, Qt::SmoothTransformation);
        } else {
            pixmap = pixmap.scaledToHeight(128, Qt::SmoothTransformation);
        }

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if ( !pixmap.save(&buffer, "PNG") ) {
            Utils::bail("Unable to save scaled avatar pixmap data", true);
            return;
        }
        if ( bytes.size() > ToxCore::MAX_AVATAR_DATA_SIZE ) {
            Utils::bail("Avatar image too large after scaling", true);
            return;
        }

        const QByteArray hash = fToxCore.hash(bytes);
        fDBData.setAvatar(-1, hash, bytes);

        emit profileAvatarChanged(hash, bytes); // sendouts are handled in friendmodel
    }

    void AvatarProvider::onAvatarFileReceived(quint32 friend_id, quint32 file_number, quint64 file_size, const QByteArray& hash)
    {
        TOX_ERR_FILE_CONTROL ctrl_error;
        TOX_FILE_CONTROL op = TOX_FILE_CONTROL_RESUME;
        quint64 transferID = Utils::transferID(friend_id, file_number);

        if ( fAvatars.value(transferID).equals(hash) || fDBData.checkAvatar(friend_id, hash) || file_size > ToxCore::MAX_AVATAR_DATA_SIZE ) {
            qDebug() << "existing avatar or file too big\n";
            op = TOX_FILE_CONTROL_CANCEL; // no need, we have this one or it's too big
        } else {
            fAvatars[transferID].init(hash);
        }

        tox_file_control(fToxCore.tox(), friend_id, file_number, op, &ctrl_error);

        if ( !Utils::handleFileControlError(ctrl_error, true) ) { // don't fail on avatar requests, just log
            qDebug() << "Unable to resume/cancel avatar download\n";
            return;
        }
    }

    void AvatarProvider::onFileChunkReceived(quint32 friend_id, quint32 file_number, quint64 position, const QByteArray &data)
    {
        Q_UNUSED(position);
        quint64 transferID = Utils::transferID(friend_id, file_number);

        if ( !fAvatars.contains(transferID) || !fAvatars.value(transferID).initialized() ) { // non-avatar chunk or cancelled
            return;
        }

        if ( data.size() == 0 ) { // done
            const Avatar& avatar = fAvatars.value(transferID);
            const QByteArray pixmapData = fAvatars.value(transferID).data();
            const QByteArray hash = fToxCore.hash(pixmapData);

            fDBData.setAvatar(friend_id, hash, pixmapData);

            if ( !avatar.isEmpty() ) {
                emit avatarChanged(friend_id);
            }
        } else {
            fAvatars[transferID].addData(data);
        }
    }

}
