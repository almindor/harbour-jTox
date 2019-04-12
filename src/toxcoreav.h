#ifndef TOXAV_H
#define TOXAV_H

#include "toxcore.h"
#include <tox/tox.h>
#include <tox/toxav.h>
#include <QMap>

namespace JTOX {

    enum MCECallState {
        csNone = 0,
        csRinging,
        csActive,
        csLast
    };

    typedef QMap<quint32, MCECallState> CallStateMap;

    class ToxCoreAV: public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int globalCallState READ getMaxGlobalState NOTIFY globalCallStateChanged)
    public:
        explicit ToxCoreAV(ToxCore& toxCore);
        virtual ~ToxCoreAV();

        void onIncomingCall(quint32 friend_id, bool audio, bool video);
        void onCallStateChanged(quint32 friend_id, quint32 state);

        Q_INVOKABLE bool answerIncomingCall(quint32 friend_id, quint32 audio_bitrate = 192);
        Q_INVOKABLE bool endCall(quint32 friend_id);
        Q_INVOKABLE bool callFriend(quint32 friend_id, quint32 audio_bitrate = 192);
    public slots:
        void onToxInitDone();
        void beforeToxKill();
    signals:
        void errorOccurred(const QString& error) const;
        void outgoingCall(quint32 friend_id) const;
        void incomingCall(quint32 friend_id, bool audio, bool video) const;
        void callStateChanged(quint32 friend_id, quint32 tav_state) const;
        void globalCallStateChanged(quint32 state) const; // MCE mapped states
    private:
        ToxCore& fToxCore;
        ToxAV* fToxAV;
        QTimer fIterationTimer;
        CallStateMap fCallStateMap;
        MCECallState fGlobalCallState;

        void initCallbacks();
        MCECallState getMaxGlobalState() const;
        void handleGlobalCallState(quint32 friend_id, MCECallState proposedState);
    };

}

#endif // TOXAV_H
