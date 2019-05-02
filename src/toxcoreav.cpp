#include "toxcoreav.h"
#include "toxcore.h"
#include "utils.h"
#include "c_callbacks.h"

namespace JTOX {

    ToxCoreAV::ToxCoreAV(ToxCore& toxCore): QObject(0),
        fToxCore(toxCore), fToxAV(nullptr),
        fCallStateMap(),
        fGlobalCallState(csNone),
        fLastCallIsIncoming(false)
    {
        // move workers to their respective threads
        fIteratorWorker.moveToThread(&fThreads[0]);
        fAudioInputWorker.moveToThread(&fThreads[1]);
        fAudioOutputWorker.moveToThread(&fThreads[2]);

        // connect worker signals and slots as needed
        connect(&fIteratorWorker, &WorkerToxAVIterator::audioFrameReceived, &fAudioOutputWorker, &WorkerAudioOutput::onAudioFrameReceived, Qt::QueuedConnection);
        connect(this, &ToxCoreAV::avIteratorStart, &fIteratorWorker, &WorkerToxAVIterator::start);
        connect(this, &ToxCoreAV::avIteratorStop, &fIteratorWorker, &WorkerToxAVIterator::stop, Qt::BlockingQueuedConnection); // needs to wait for it
        connect(this, &ToxCoreAV::startAudio, &fAudioInputWorker, &WorkerAudioInput::start);
        connect(this, &ToxCoreAV::stopAudio, &fAudioInputWorker, &WorkerAudioInput::stop, Qt::BlockingQueuedConnection); // needs to wait for it
        connect(this, &ToxCoreAV::startAudio, &fAudioOutputWorker, &WorkerAudioOutput::start);
        connect(this, &ToxCoreAV::stopAudio, &fAudioOutputWorker, &WorkerAudioOutput::stop, Qt::BlockingQueuedConnection); // needs to wait for it

        for (int i = 0; i < 3; i++) {
            fThreads[i].start();
        }
    }

    ToxCoreAV::~ToxCoreAV()
    {
        qDebug() << "~ToxCoreAV()";
        // should be called by App::lastWindowsClosed() signal but in case it got missed somehow make sure to cleanup in order
        beforeToxKill();

        for (int i = 0; i < 3; i++) {
            fThreads[i].quit();
            if (!fThreads[i].wait(2000)) {
                qWarning() << "Thread misbehaving on quit";
            }
        }
    }

    void ToxCoreAV::onIncomingCall(quint32 friend_id, bool audio, bool video)
    {
        qDebug() << "Incoming call!";
        fLastCallIsIncoming = true;

        TOXAV_ERR_CALL_CONTROL error;

        // disable video right away until we support it
        if (video) {
            toxav_call_control(fToxAV, friend_id, TOXAV_CALL_CONTROL_HIDE_VIDEO, &error);
            const QString errorStr = Utils::handleToxAVControlError(error);

            if (!errorStr.isEmpty()) {
                emit errorOccurred(errorStr);
            } else {
                video = false;
            }
        }

        handleGlobalCallState(friend_id, csRinging, false);
        emit incomingCall(friend_id, audio, video);
    }

    void ToxCoreAV::onCallStateChanged(quint32 friend_id, quint32 tav_state)
    {
        MCECallState state = tav_state > 2 ? csActive : csNone;

        handleGlobalCallState(friend_id, state, false); // in call or finished/error/none
    }

    bool ToxCoreAV::answerIncomingCall(quint32 friend_id, quint32 audio_bitrate)
    {
        if (fToxAV == nullptr) {
            Utils::fatal("ToxAV not initialized");
        }

        TOXAV_ERR_ANSWER error;
        bool result = toxav_answer(fToxAV, friend_id, audio_bitrate, 0, &error);
        const QString errorStr = Utils::handleToxAVAnswerError(error);

        if (!errorStr.isEmpty()) {
            emit errorOccurred(errorStr);
            return false;
        }

        handleGlobalCallState(friend_id, result ? csActive : csNone, true);

        return result;
    }

    bool ToxCoreAV::endCall(quint32 friend_id)
    {
        if (fToxAV == nullptr) {
            Utils::fatal("ToxAV not initialized");
        }

        // make sure there's no sending or receiving running after we call control
        emit stopAudio();

        TOXAV_ERR_CALL_CONTROL error;
        bool result = toxav_call_control(fToxAV, friend_id, TOXAV_CALL_CONTROL_CANCEL, &error);

        const QString errorStr = Utils::handleToxAVControlError(error);
        if (!errorStr.isEmpty()) {
            emit errorOccurred(errorStr);
            return false;
        }

        if (result) {
            handleGlobalCallState(friend_id, csNone, true);
        }

        return result;
    }

    bool ToxCoreAV::callFriend(quint32 friend_id, quint32 audio_bitrate)
    {
        if (fToxAV == nullptr) {
            Utils::fatal("ToxAV not initialized");
        }

        TOXAV_ERR_CALL error;
        bool result = toxav_call(fToxAV, friend_id, audio_bitrate, 0, &error);
        const QString errorStr = Utils::handleToxAVCallError(error);

        if (!errorStr.isEmpty()) {
            emit errorOccurred(errorStr);

            return false;
        }

        if (result) {
            emit outgoingCall(friend_id);
            handleGlobalCallState(friend_id, csRinging, true);
        }

        return result;
    }

    void ToxCoreAV::onToxInitDone()
    {
        if (fToxAV != nullptr) {
            Utils::fatal("onToxInitDone called when AV still initialized");
        }

        if (fToxCore.tox() == nullptr) {
            Utils::fatal("Tox core not initialized when attempting A/V init");
        }

        TOXAV_ERR_NEW error;
        fToxAV = toxav_new(fToxCore.tox(), &error);
        Utils::handleToxAVNewError(error);

        initCallbacks();
        emit avIteratorStart(fToxAV);
    }

    void ToxCoreAV::beforeToxKill()
    {
        if (fToxAV == nullptr) {
            return;
        }

        qDebug() << "beforeToxKill()";

        emit stopAudio();
        emit avIteratorStop();

        toxav_kill(fToxAV);
        fToxAV = nullptr;
    }

    void ToxCoreAV::initCallbacks()
    {
        toxav_callback_call(fToxAV, c_toxav_call_cb, this);
        toxav_callback_call_state(fToxAV, c_toxav_call_state_cb, this);
        toxav_callback_audio_bit_rate(fToxAV, c_toxav_audio_bit_rate_cb, this);
        toxav_callback_audio_receive_frame(fToxAV, c_toxav_audio_receive_frame_cb, &fIteratorWorker); // make sure to call callback in same object/thread
    }

    MCECallState ToxCoreAV::getMaxGlobalState() const
    {
        MCECallState maxState = csNone;
        foreach (MCECallState state, fCallStateMap.values()) {
            if (state > maxState) {
                maxState = state;
            }

            if (maxState == csActive) {
                return maxState;
            }
        }

        return maxState;
    }

    bool ToxCoreAV::getCallIsIncoming() const
    {
        return fLastCallIsIncoming;
    }

    void ToxCoreAV::handleGlobalCallState(quint32 friend_id, MCECallState proposedState, bool local)
    {
        if (proposedState == csNone) {
            fCallStateMap.remove(friend_id); // clean up
        } else {
            fCallStateMap[friend_id] = proposedState;
        }

        MCECallState maxState = getMaxGlobalState();

        if (maxState != fGlobalCallState) {
            if (maxState == csActive) { // start of call
                emit startAudio(fToxAV, friend_id);
            } else if (fGlobalCallState == csActive) { // end of call
                emit stopAudio();
            }

//            if (maxState > csNone) { // going into ringing or in call
//                emit avIteratorStart(fToxAV);
//            } else { // stopping ringing or off call
//                emit avIteratorStop();
//            }

            if (maxState == csNone) {
                if (!fLastCallIsIncoming && !local) {
                    emit calledBusy();
                }

                fLastCallIsIncoming = false; // reset
            }

            fGlobalCallState = maxState;
            emit globalCallStateChanged(fGlobalCallState);
        }

        emit callStateChanged(friend_id, proposedState, local);
    }

}
