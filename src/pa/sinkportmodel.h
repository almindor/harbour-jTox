#ifndef SINKPORTMODEL_H
#define SINKPORTMODEL_H

#include "context.h"
#include <QObject>
#include <QAbstractListModel>
#include <pulse/introspect.h>

namespace PA {

    enum AudioPorts {
        apEarpiece = 0,
        apSpeaker
    };

    class SinkPortModel : public QAbstractListModel
    {
        Q_OBJECT

        pa_context* fContext{nullptr};
        SinkInfo fSinkInfo;

        void selectPort(const QString& name);
    public:
        explicit SinkPortModel();

        int rowCount(const QModelIndex &parent) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        bool ready() const;

        Q_INVOKABLE void selectPort(int port);
    public slots:
        void onInfoReady(void* context, const SinkInfo& info);
        void contextCallResult(int result);
    };

}

#endif // SINKPORTMODEL_H
