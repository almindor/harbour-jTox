#ifndef REQUESTMODEL_H
#define REQUESTMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include "toxcore.h"
#include "toxme.h"
#include "friendrequest.h"
#include "friendmodel.h"
#include "dbdata.h"

namespace JTOX {

    class RequestModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int size READ getSize NOTIFY sizeChanged)
    public:
        RequestModel(const ToxCore& toxcore, Toxme& toxme, FriendModel& friendModel, DBData& dbData);

        QHash<int, QByteArray> roleNames() const;
        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
        Q_INVOKABLE void accept(int index);
        Q_INVOKABLE void reject(int index);
    public slots:
        void onFriendRequest(const QString& publicKey, const QString& message);
        void onReverseLookupDone(const QString& publicKey, const QString& name, int requestID);
        void onReverseLookupError(const QString& error, const QString& userInfo, int requestID);
        void refresh();
    signals:
        void sizeChanged(int size);
    private:
        RequestList fList;
        Toxme& fToxme;
        FriendModel& fFriendModel;
        DBData& fDBData;
        int fLookupID;

        int getSize() const;
    };

}

#endif // REQUESTMODEL_H
