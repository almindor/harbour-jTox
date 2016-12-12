#include "dirmodel.h"
#include <QStandardPaths>

namespace JTOX {

    DirModel::DirModel() : QAbstractListModel(0)
    {
        const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        fDir = QDir(dirPath);
        fWatcher.addPath(dirPath);

        QStringList filters;
        filters << "*.tox";
        fDir.setNameFilters(filters);

        connect(&fWatcher, &QFileSystemWatcher::directoryChanged, this, &DirModel::refresh);
    }

    int DirModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);
        return fDir.entryList().size();
    }

    QHash<int, QByteArray> DirModel::roleNames() const
    {
        QHash<int, QByteArray> result;
        result[dmrFileName] = "fileName";

        return result;

    }

    QVariant DirModel::data(const QModelIndex& index, int role) const
    {
        Q_UNUSED(role);
        return fDir.entryList().at(index.row());
    }

    void DirModel::refresh(const QString& path)
    {
        Q_UNUSED(path);

        beginResetModel();
        fDir.refresh();
        endResetModel();
    }

}
