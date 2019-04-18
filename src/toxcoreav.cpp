#include "toxcoreav.h"
#include "toxcore.h"
#include "utils.h"
#include "c_callbacks.h"

namespace JTOX {

    ToxCoreAV::ToxCoreAV(ToxCore& toxCore): QThread(0),
        fToxCore(toxCore), fToxAV(nullptr),
        fCallStateMap(),
        fGlobalCallState(csNone),
        fCall(),
        fActive(false)
    {
    }

    ToxCoreAV::~ToxCoreAV()
    {
        // stack deconstructs our object before ToxCore thus we never get beforeToxKill() in case of app shutdown
        // handle this case from destructor here
        beforeToxKill();
    }

    void ToxCoreAV::onIncomingCall(quint32 friend_id, bool audio, bool video)
    {
        qDebug() << "Incoming call!\n";
        if (fCall.isRunning()) {
            endCall(friend_id);
        }

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

        handleGlobalCallState(friend_id, csRinging);
        emit incomingCall(friend_id, audio, video);
    }

    void ToxCoreAV::onCallStateChanged(quint32 friend_id, quint32 tav_state)
    {
        MCECallState state = tav_state > 2 ? csActive : csNone;

        handleGlobalCallState(friend_id, state); // in call or finished/error/none
        emit callStateChanged(friend_id, state);
    }

    void ToxCoreAV::onAudioFrameReceived(quint32 friend_id, const qint16* pcm, size_t sample_count, quint8 channels, quint32 sampling_rate)
    {
        if (fCall.friendID() != friend_id) {
            qWarning() << "Mismatched friend_id between call and toxcoreav";
            return;
        }

        const QByteArray data = QByteArray::fromRawData((char*) pcm, sample_count * channels * 2); // copy as bytes so we can juggle between threads
        fCall.handleIncomingFrame(data, channels, sampling_rate);
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

        handleGlobalCallState(friend_id, result ? csActive : csNone);
        callStateChanged(friend_id, result ? csActive : csNone);

        return result;
    }

    bool ToxCoreAV::endCall(quint32 friend_id)
    {
        if (fToxAV == nullptr) {
            Utils::fatal("ToxAV not initialized");
        }

        if (fCall.isRunning() && friend_id == fCall.friendID()) {
            fCall.finish();
        }

        TOXAV_ERR_CALL_CONTROL error;
        bool result = toxav_call_control(fToxAV, friend_id, TOXAV_CALL_CONTROL_CANCEL, &error);

        const QString errorStr = Utils::handleToxAVControlError(error);
        if (!errorStr.isEmpty()) {
            emit errorOccurred(errorStr);
            return false;
        }

        if (result) {
            handleGlobalCallState(friend_id, csNone);
            emit callStateChanged(friend_id, csNone);
        }

        return result;
    }

    bool ToxCoreAV::callFriend(quint32 friend_id, quint32 audio_bitrate)
    {
        if (fToxAV == nullptr) {
            Utils::fatal("ToxAV not initialized");
        }

        if (fCall.isRunning()) {
            emit errorOccurred("Call in progress");
            return false;
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
            handleGlobalCallState(friend_id, csRinging);
            emit callStateChanged(friend_id, csRinging);
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
        fActive = true;
        start();
    }

    void ToxCoreAV::beforeToxKill()
    {
        if (fToxAV == nullptr) {
            return;
        }

        fActive = false;
        // cleanly end call and all audio resources
        if (fCall.isRunning()) {
            fCall.finish();
        }

        // wait for toxav_iteration thread
        if (!wait(2000)) {
            qWarning() << "Unable to cleanly exit toxav iteration thread";
            terminate();
        }

        toxav_kill(fToxAV);
        fToxAV = nullptr;
    }

    void ToxCoreAV::initCallbacks()
    {
        toxav_callback_call(fToxAV, c_toxav_call_cb, this);
        toxav_callback_call_state(fToxAV, c_toxav_call_state_cb, this);
        toxav_callback_audio_bit_rate(fToxAV, c_toxav_audio_bit_rate_cb, this);
        toxav_callback_audio_receive_frame(fToxAV, c_toxav_audio_receive_frame_cb, this);
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

    void ToxCoreAV::handleGlobalCallState(quint32 friend_id, MCECallState proposedState)
    {
        if (proposedState == csNone) {
            fCallStateMap.remove(friend_id); // clean up
        } else {
            fCallStateMap[friend_id] = proposedState;
        }

        MCECallState maxState = getMaxGlobalState();

        if (maxState != fGlobalCallState) {
            if (maxState == csActive) {
                if (!fCall.init(fToxAV, friend_id)) {
                    qWarning() << "Unable to initialize Call processor";
                    emit errorOccurred("Unable to initialize Call processor");
                }
            } else if (fGlobalCallState == csActive) {
                fCall.finish();
            }

            fGlobalCallState = maxState;
            emit globalCallStateChanged(fGlobalCallState);
        }
    }

    void ToxCoreAV::run()
    {
        if (fToxAV == nullptr) {
            qWarning() << "toxav_iterate called with null toxav\n";
            return;
        }

        while (fActive) {
            toxav_iterate(fToxAV);
            QThread::msleep(toxav_iteration_interval(fToxAV));
        }
    }


}
