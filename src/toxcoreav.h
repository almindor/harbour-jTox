#ifndef TOXAV_H
#define TOXAV_H

#include "toxcore.h"
#include "call.h"
#include <tox/tox.h>
#include <tox/toxav.h>
#include <QThread>
#include <QMap>
#include <QAudioInput>
#include <QAudioOutput>

namespace JTOX {

    constexpr int DEFAULT_BITRATE = 24;
    constexpr int DEFAULT_OOC_DELAY = 2000; // 2s to save battery when out of call

    enum MCECallState {
        csNone = 0,
        csRinging,
        csActive,
        csLast
    };

    typedef QMap<quint32, MCECallState> CallStateMap;

    class ToxCoreAV: public QThread
    {
        Q_OBJECT
        Q_PROPERTY(int globalCallState READ getMaxGlobalState NOTIFY globalCallStateChanged)
    public:
        explicit ToxCoreAV(ToxCore& toxCore);
        virtual ~ToxCoreAV();

        void onIncomingCall(quint32 friend_id, bool audio, bool video);
        void onCallStateChanged(quint32 friend_id, quint32 state);
        void onAudioFrameReceived(quint32 friend_id, const qint16* pcm, size_t sample_count, quint8 channels, quint32 sampling_rate);

        Q_INVOKABLE bool answerIncomingCall(quint32 friend_id, quint32 audio_bitrate = DEFAULT_BITRATE);
        Q_INVOKABLE bool endCall(quint32 friend_id);
        Q_INVOKABLE bool callFriend(quint32 friend_id, quint32 audio_bitrate = DEFAULT_BITRATE);
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
        CallStateMap fCallStateMap;
        MCECallState fGlobalCallState;
        Call fCall; // running call singleton TODO: move into callstatemap for multiple simultanious calls support
        bool fActive;

        void initCallbacks();
        MCECallState getMaxGlobalState() const;
        void handleGlobalCallState(quint32 friend_id, MCECallState proposedState);

        void run() override;
    };

}

#endif // TOXAV_H
