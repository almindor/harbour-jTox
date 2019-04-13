#include "toxcoreav.h"
#include "toxcore.h"
#include "utils.h"
#include "c_callbacks.h"

namespace JTOX {

    ToxCoreAV::ToxCoreAV(ToxCore& toxCore): QObject(0),
        fToxCore(toxCore), fToxAV(nullptr),
        fIterationTimer(), fCallStateMap(),
        fGlobalCallState(csNone)
    {
        fIterationTimer.setInterval(200); // TODO
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

        TOXAV_ERR_CALL error;
        bool result = toxav_call(fToxAV, friend_id, audio_bitrate, 0, &error);
        const QString errorStr = Utils::handleTOXAVCallError(error);

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
        fIterationTimer.start();
    }

    void ToxCoreAV::beforeToxKill()
    {
        if (fToxAV == nullptr) {
            return;
        }

        fIterationTimer.stop();

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

        qDebug() << "global max state: " << maxState << "\n";

        if (maxState != fGlobalCallState) {
            fGlobalCallState = maxState;
            emit globalCallStateChanged(fGlobalCallState);
        }
    }

}
