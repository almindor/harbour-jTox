#ifndef CONTEXT_H
#define CONTEXT_H

#include <QObject>
#include <QThread>
#include <QStringList>
#include <pulse/mainloop.h>
#include <pulse/context.h>
#include <pulse/introspect.h>

namespace PA {

    class SinkInfo
    {
        QString fName;
        quint32 fIndex;
        QStringList fPorts;
    public:
        explicit SinkInfo();
        explicit SinkInfo(const pa_sink_info* source);

        quint32 index() const;
        QString name() const;
        QStringList ports() const;
    };

    class MainLoop : public QThread
    {
        Q_OBJECT

        pa_mainloop* fMainLoop;

        void run() override;
    public:
        explicit MainLoop();
        virtual ~MainLoop();

        pa_mainloop* mainLoop() const;
        void shutdown();
    };

    class Context : public QObject
    {
        Q_OBJECT

        MainLoop fMainLoop;
        pa_context* fContext;
        const pa_card_info* fPrimaryCardInfo{nullptr};
        const pa_sink_info* fPrimarySinkInfo{nullptr};

        void onContextReady();
        void checkInfoReady() const;
    public:
        explicit Context(QObject *parent = nullptr);
        virtual ~Context();

        void connectToServer(const QString& path = QString());
    signals:
        void errorOccurred(const QString& error) const;
        void infoReady(void* context, const SinkInfo& info) const; // use void* to avoid having to register stuff
    public slots:
        void onStateChanged(pa_context_state_t state);
        void onCardInfo(const pa_card_info* info);
        void onSinkInfo(const pa_sink_info* info);
    };

}

#endif // CONTEXT_H
