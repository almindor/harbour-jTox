#ifndef DIRMODEL_H
#define DIRMODEL_H

#include <QDir>
#include <QAbstractListModel>
#include <QFileSystemWatcher>

namespace JTOX {

    enum DirModelRoles {
        dmrFileName
    };

    class DirModel : public QAbstractListModel
    {
    public:
        DirModel();

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        QHash<int, QByteArray> roleNames() const;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    private:
        QDir fDir;
        QFileSystemWatcher fWatcher;
    private slots:
        void refresh(const QString& path);
    };

}

#endif // DIRMODEL_H
