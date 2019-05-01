#include "context.h"
#include <QDebug>

namespace PA {

    //---------------------- SinkInfo ------------------------//=

    SinkInfo::SinkInfo() :
        fName(), fIndex(-1), fPorts()
    {

    }

    SinkInfo::SinkInfo(const pa_sink_info* source)
    {
        fName = source->name;
        fIndex = source->index;

        for (quint32 i = 0; i < source->n_ports; i++) {
            fPorts.append(source->ports[i]->name);
        }
    }

    quint32 SinkInfo::index() const
    {
        return fIndex;
    }

    QString SinkInfo::name() const
    {
        return fName;
    }

    QStringList SinkInfo::ports() const
    {
        return fPorts;
    }

    //---------------------- MainLoop ------------------------//

    void MainLoop::run()
    {
        int retval = 0;
        int result = 0;
        do
        {
            result = pa_mainloop_iterate(fMainLoop, true, &retval);
        } while(result >= 0);

        qWarning() << "PA loop end code" << retval;
    }

    pa_mainloop*MainLoop::mainLoop() const
    {
        return fMainLoop;
    }

    void MainLoop::shutdown()
    {
        pa_mainloop_quit(fMainLoop, 200);
    }

    MainLoop::MainLoop() : QThread(0),
      fMainLoop(pa_mainloop_new())
    {

    }

    MainLoop::~MainLoop()
    {
        pa_mainloop_free(fMainLoop);
    }

    //---------------------- C callbacks --------------------//

    void c_pa_context_notify_cb_t(pa_context *c, void *userdata) {
        Context* context = (Context*) userdata;

        context->onStateChanged(pa_context_get_state(c));
    }

    void c_pa_sink_info_cb_t(pa_context *c, const pa_sink_info *ci, int eol, void *userdata) {
        Q_UNUSED(c);
        Q_UNUSED(eol);
        Context* context = (Context*) userdata;

        if (ci == nullptr) {
            return;
        }

        if (ci->n_ports > 0) {
            context->onSinkInfo(ci);
        }
    }

    void c_pa_card_info_cb_t(pa_context *c, const pa_card_info* ci, int eol, void *userdata) {
        Q_UNUSED(c);
        Q_UNUSED(eol);
        Context* context = (Context*) userdata;

        if (ci == nullptr) {
            return;
        }

        if (ci->n_ports > 0) {
            context->onCardInfo(ci);
        }
    }

    //---------------------- Context ------------------------//

    void Context::onContextReady()
    {
        qDebug() << "Server" << pa_context_get_server(fContext);

        pa_context_get_card_info_list(fContext, &c_pa_card_info_cb_t, this);
        pa_context_get_sink_info_list(fContext, &c_pa_sink_info_cb_t, this);
    }

    void Context::checkInfoReady() const
    {
        if (fPrimaryCardInfo == nullptr || fPrimarySinkInfo == nullptr) {
            return;
        }

        emit infoReady(fContext, SinkInfo(fPrimarySinkInfo));
    }

    Context::Context(QObject *parent) : QObject(parent),
        fMainLoop(),
        fContext(nullptr)
    {
        fContext = pa_context_new(pa_mainloop_get_api(fMainLoop.mainLoop()), "harbour-jTox");
        pa_context_set_state_callback(fContext, &c_pa_context_notify_cb_t, this);

        fMainLoop.start();
    }

    Context::~Context()
    {
        fMainLoop.shutdown(); // pa shutdown
        if (!fMainLoop.wait(2000)) {
            fMainLoop.terminate();
        }
    }

    void Context::connectToServer(const QString& path)
    {
        const char* serverPath = path.isEmpty() ? nullptr : path.toLocal8Bit().constData();

        if (pa_context_connect(fContext, serverPath, PA_CONTEXT_NOAUTOSPAWN, nullptr) < 0) {
            qWarning() << "Error connecting pa context to pa server" << pa_context_errno(fContext); // TODO: error handling
            return;
        }
    }

    void Context::onStateChanged(pa_context_state_t state)
    {
        if (state == PA_CONTEXT_TERMINATED || state == PA_CONTEXT_FAILED) {
            emit errorOccurred("PA entered invalid state");
            return;
        }

        if (state == PA_CONTEXT_READY) {
            onContextReady();
        }
    }

    void Context::onCardInfo(const pa_card_info* info)
    {
        // sanity check
        const QString name = QString::fromLocal8Bit(info->name);
        if (name.contains("null") || name.contains("fake")) {
            return; // not real
        }

        if (fPrimaryCardInfo != nullptr) {
            qWarning() << "duplicit card found" << name;
        }

        fPrimaryCardInfo = info;
        qDebug() << "Found primary audio card" << name;
        checkInfoReady();
    }

    void Context::onSinkInfo(const pa_sink_info* info)
    {
        // sanity check
        const QString name = QString::fromLocal8Bit(info->name);
        if (name != "sink.primary") {
            return; // not real
        }

        if (fPrimarySinkInfo != nullptr) {
            qWarning() << "duplicit sink found" << name;
        }

        fPrimarySinkInfo = info;
        qDebug() << "Found primary sink" << name;
        checkInfoReady();
    }

}
