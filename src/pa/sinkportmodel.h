#ifndef SINKPORTMODEL_H
#define SINKPORTMODEL_H

#include "context.h"
#include <QObject>
#include <pulse/introspect.h>

namespace PA {

    enum AudioPorts {
        apEarpiece = 0,
        apSpeaker
    };

    class SinkPortModel : public QObject
    {
        Q_OBJECT

        pa_context* fContext{nullptr};
        QString fCardName;
        QString fSinkName;

        void selectPort(const QString& name);
    public:
        explicit SinkPortModel();
        bool ready() const;

        Q_INVOKABLE void selectPort(int port);
        Q_INVOKABLE void selectProfile(const QString& name);
    public slots:
        void onInfoReady(void* context, const QString& cardName, const QString& sinkName);
        void selectProfileResult(int result);
        void setSinkPortResult(int result);
    };

}

#endif // SINKPORTMODEL_H
