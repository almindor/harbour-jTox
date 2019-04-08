#include "toxcoreav.h"
#include "toxcore.h"
#include "utils.h"
#include "c_callbacks.h"

namespace JTOX {

    ToxCoreAV::ToxCoreAV(ToxCore& toxCore): QObject(0),
        fToxCore(toxCore), fToxAV(nullptr)
    {

    }

    ToxCoreAV::~ToxCoreAV()
    {
        // stack deconstructs our object before ToxCore thus we never get beforeToxKill() in case of app shutdown
        // handle this case from destructor here
        beforeToxKill();
    }

    void ToxCoreAV::onIncomingCall(quint32 friend_id, bool audio, bool video) const
    {
        TOXAV_ERR_CALL_CONTROL error;

        // disable video right away until we support it
        if (video) {
            toxav_call_control(fToxAV, friend_id, TOXAV_CALL_CONTROL_HIDE_VIDEO, &error);
            QString errorStr = Utils::handleToxAVControlError(error);

            if (!errorStr.isEmpty()) {
                emit errorOccurred(errorStr);
            } else {
                video = false;
            }
        }

        emit incomingCall(friend_id, audio, video);
    }

    void ToxCoreAV::onCallStateChanged(quint32 friend_id, quint32 state) const
    {
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
    }

    void ToxCoreAV::beforeToxKill()
    {
        if (fToxAV == nullptr) {
            return;
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

}
