#ifndef TOXAV_H
#define TOXAV_H

#include "toxcore.h"
#include <tox/tox.h>
#include <tox/toxav.h>

namespace JTOX {

    class ToxCoreAV: public QObject
    {
        Q_OBJECT
    public:
        explicit ToxCoreAV(ToxCore& toxCore);
        virtual ~ToxCoreAV();

        void onIncomingCall(quint32 friend_id, bool audio, bool video) const;
        void onCallStateChanged(quint32 friend_id, quint32 state) const;

        Q_INVOKABLE bool answerIncomingCall(quint32 friend_id, quint32 audio_bitrate = 192);
        Q_INVOKABLE bool callFriend(quint32 friend_id, quint32 audio_bitrate = 192);
    public slots:
        void onToxInitDone();
        void beforeToxKill();
    signals:
        void errorOccurred(const QString& error) const;
        void incomingCall(quint32 friend_id, bool audio, bool video) const;
        void callStateChanged(quint32 friend_id, quint32 state) const;
    private:
        ToxCore& fToxCore;
        ToxAV* fToxAV;

        void initCallbacks();
    };

}

#endif // TOXAV_H
