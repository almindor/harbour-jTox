#include "requestmodel.h"
#include "utils.h"
#include <QStringList>
#include <QSettings>
#include <QVector>
#include <QDebug>

namespace JTOX {

    RequestModel::RequestModel(const ToxCore& toxcore, Toxme& toxme, FriendModel& friendModel, DBData& dbData) :
            QAbstractListModel(0), fList(), fToxme(toxme), fFriendModel(friendModel), fDBData(dbData),
            fLookupID(-1)
    {
        connect(&toxcore, &ToxCore::clientReset, this, &RequestModel::refresh);
        connect(&toxcore, &ToxCore::friendRequest, this, &RequestModel::onFriendRequest);
        connect(&toxme, &Toxme::reverseLookupDone, this, &RequestModel::onReverseLookupDone);        
    }

    QHash<int, QByteArray> RequestModel::roleNames() const {
        QHash<int, QByteArray> result;
        result[rrPublicKey] = "address";
        result[rrMessage] = "message";
        result[rrName] = "name";

        return result;
    }

    int RequestModel::rowCount(const QModelIndex &parent) const {
        Q_UNUSED(parent);
        return fList.size();
    }

    QVariant RequestModel::data(const QModelIndex &index, int role) const {
        if ( index.row() < 0 || index.row() >= fList.size() ) {
            Utils::fatal("Friend request data index out of bounds");
        }

        return fList.at(index.row()).value(role);
    }

    void RequestModel::accept(int index) {
        if ( index < 0 || index >= fList.size() ) {
            return;
        }

        const QString address = fList.at(index).value(rrPublicKey).toString();
        const QString name = fList.at(index).value(rrName).toString();
        fFriendModel.addFriendNoRequest(address, name);
        reject(index); // removes from list
    }

    void RequestModel::reject(int index) {
        if ( index < 0 || index >= fList.size() ) {
            return;
        }

        beginRemoveRows(QModelIndex(), index, index);
        fDBData.deleteRequest(fList.at(index));
        fList.removeAt(index);
        endRemoveRows();
        emit sizeChanged(fList.size());
    }

    void RequestModel::onFriendRequest(const QString &publicKey, const QString &message)
    {
        // we already have it
        foreach ( const FriendRequest& request, fList ) {
            if ( request.value(rrPublicKey).toString() == publicKey ) {
                if ( request.value(rrName).toString().isEmpty() ) { // try to get name if we don't have it even if stored
                    fLookupID = fToxme.reverseLookup(publicKey); // fire this off right away
                }
                return;
            }
        }

        fLookupID = fToxme.reverseLookup(publicKey); // fire this off right away

        beginInsertRows(QModelIndex(), fList.size(), fList.size());
        fList.append(FriendRequest(publicKey, message, ""));
        fDBData.insertRequest(fList.last());
        endInsertRows();
        emit sizeChanged(fList.size());
    }

    void RequestModel::onReverseLookupDone(const QString& publicKey, const QString& name, int requestID) {
        if ( requestID != fLookupID ) {
            return; // not ours
        }

        int i = 0;
        foreach ( const FriendRequest& request, fList ) {
            if ( request.value(rrPublicKey).toString() == publicKey ) {
                fList[i].setName(name);
                fDBData.updateRequest(fList.at(i));

                emit dataChanged(createIndex(i, 0), createIndex(i, 0), QVector<int>(1, rrName));

                return;
            }

            i++;
        }
    }

    void RequestModel::onReverseLookupError(const QString &error, const QString &userInfo, int requestID)
    {
        Q_UNUSED(userInfo);

        if ( requestID != fLookupID ) {
            return; // not ours
        }

        qDebug() << "Error from toxme: " << error << "\n";
    }

    void RequestModel::refresh()
    {
        beginResetModel();

        // check old request storage and migrate to DB
        QSettings settings;
        settings.beginGroup("app/friends/requests");
        const QStringList addresses = settings.childGroups();

        foreach ( const QString& address, addresses ) {
            const QString message = settings.value(address + "/message").toString();
            const QString name = settings.value(address + "/name").toString();

            FriendRequest request(address, message, name);
            fDBData.insertRequest(request);
        }
        settings.endGroup();
        settings.remove("app/friends/requests"); // wipe old storage after migration

        // get requests from DB
        fDBData.getRequests(fList);

        endResetModel();
        emit sizeChanged(fList.size());
    }

    int RequestModel::getSize() const
    {
        return fList.size();
    }



}
