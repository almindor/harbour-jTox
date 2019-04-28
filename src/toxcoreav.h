#ifndef TOXAV_H
#define TOXAV_H

#include "toxcore.h"
#include "workerav.h"
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

    class ToxCoreAV: public QObject
    {
        Q_OBJECT
        Q_PROPERTY(int globalCallState READ getMaxGlobalState NOTIFY globalCallStateChanged)
        Q_PROPERTY(bool callIsIncoming READ getCallIsIncoming NOTIFY globalCallStateChanged)
    public:
        explicit ToxCoreAV(ToxCore& toxCore);
        virtual ~ToxCoreAV();

        void onIncomingCall(quint32 friend_id, bool audio, bool video);
        void onCallStateChanged(quint32 friend_id, quint32 state);

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
        void callStateChanged(quint32 friend_id, quint32 tav_state, bool local) const;
        void globalCallStateChanged(quint32 state) const; // MCE mapped states
        void calledBusy() const; // when other side cuts us off
        // worker signals
        void avIteratorStart(void* toxAV) const;
        void avIteratorStop() const;
        void startAudio(void* toxAV, quint32 friend_id) const;
        void stopAudio() const;
    private:
        ToxCore& fToxCore;
        ToxAV* fToxAV;
        CallStateMap fCallStateMap;
        MCECallState fGlobalCallState;
        QThread fThreads[3]; // worker threads
        WorkerToxAVIterator fIteratorWorker;
        WorkerAudioInput fAudioInputWorker;
        WorkerAudioOutput fAudioOutputWorker;
        bool fLastCallIsIncoming;

        void initCallbacks();
        MCECallState getMaxGlobalState() const;
        bool getCallIsIncoming() const;
        void handleGlobalCallState(quint32 friend_id, MCECallState proposedState, bool local);
    };

}

#endif // TOXAV_H
