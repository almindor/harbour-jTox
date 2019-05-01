#include "sinkportmodel.h"
#include <QDebug>

namespace PA {

    // -------------------- C callbacks -----------------------/

    void c_pa_context_success_cb_t(pa_context *c, int success, void *userdata) {
        Q_UNUSED(c);
        SinkPortModel* model = (SinkPortModel*) userdata;

        model->contextCallResult(success);
    }

    // -------------------- SinkPortModel -----------------------/

    void SinkPortModel::selectPort(const QString& name)
    {
        qDebug() << "Selecting port" << name << "on sink" << fSinkInfo.name();
        pa_context_set_sink_port_by_name(fContext, fSinkInfo.name().toLocal8Bit().constData(), name.toLocal8Bit().constData(), &c_pa_context_success_cb_t, this);
    }

    SinkPortModel::SinkPortModel() : QAbstractListModel(nullptr),
        fSinkInfo()
    {

    }

    int SinkPortModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent);

        if (!ready()) {
            return 0;
        }

        return fSinkInfo.ports().count();
    }

    QVariant SinkPortModel::data(const QModelIndex& index, int role) const
    {
        Q_UNUSED(role);

        return fSinkInfo.ports().at(index.row());
    }

    bool SinkPortModel::ready() const
    {
        return !fSinkInfo.name().isEmpty();
    }

    void SinkPortModel::selectPort(int port)
    {
        if (!ready()) {
            qWarning() << "PA not ready yet";
            return;
        }

        switch (port) {
            case apEarpiece: return selectPort("output-earpiece");
            case apSpeaker: return selectPort("output-speaker");
        }

        qWarning() << "Unknown port requested";
    }

    void SinkPortModel::onInfoReady(void* context, const SinkInfo& info)
    {
        beginResetModel();

        fContext = (pa_context*) context;
        fSinkInfo = info;

        endResetModel();

        selectPort(apSpeaker); // default on start
    }

    void SinkPortModel::contextCallResult(int result)
    {
        qWarning() << "Context call result" << result;
    }

}
