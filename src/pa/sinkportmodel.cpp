#include "sinkportmodel.h"
#include <QDebug>

namespace PA {

    // -------------------- C callbacks -----------------------/

    void c_pa_context_set_sink_port_cb_t(pa_context *c, int success, void *userdata) {
        Q_UNUSED(c);
        SinkPortModel* model = (SinkPortModel*) userdata;

        model->setSinkPortResult(success);
    }

    void c_pa_context_select_profile_cb_t(pa_context *c, int success, void *userdata) {
        Q_UNUSED(c);
        SinkPortModel* model = (SinkPortModel*) userdata;

        model->selectProfileResult(success);
    }

    // -------------------- SinkPortModel -----------------------/

    void SinkPortModel::selectPort(const QString& name)
    {
        qDebug() << "[PA] Selecting port" << name << "on sink" << fSinkName;
        pa_context_set_sink_port_by_name(fContext, fSinkName.toLocal8Bit().constData(), name.toLocal8Bit().constData(), &c_pa_context_set_sink_port_cb_t, this);
    }

    void SinkPortModel::selectProfile(const QString& name)
    {
        qDebug() << "[PA] Selecting profile" << name << "on card" << fCardName;

        pa_context_set_card_profile_by_name(fContext, fCardName.toLocal8Bit().constData(), name.toLocal8Bit().constData(), &c_pa_context_select_profile_cb_t, this);
    }

    SinkPortModel::SinkPortModel() : QObject(nullptr),
        fCardName(), fSinkName()
    {

    }

    bool SinkPortModel::ready() const
    {
        return !fCardName.isEmpty() && !fSinkName.isEmpty();
    }

    void SinkPortModel::selectPort(int port)
    {
        if (!ready()) {
            qWarning() << "[PA] not ready yet";
            return;
        }

        switch (port) {
            case apEarpiece: return selectPort("output-earpiece");
            case apSpeaker: return selectPort("output-speaker");
        }

        qWarning() << "[PA] Unknown port requested";
    }

    void SinkPortModel::onInfoReady(void* context, const QString& cardName, const QString& sinkName)
    {
        fContext = (pa_context*) context;
        fCardName = cardName;
        fSinkName = sinkName;

        selectProfile("default"); // default on start
        selectPort(apSpeaker); // default on start
    }

    void SinkPortModel::selectProfileResult(int result)
    {
        qWarning() << "[PA] select profile result" << result;
    }

    void SinkPortModel::setSinkPortResult(int result)
    {
        qWarning() << "[PA] set sink port result" << result;
    }

}
