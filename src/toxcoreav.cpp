#include "toxcoreav.h"
#include "toxcore.h"
#include "utils.h"
#include "c_callbacks.h"

namespace JTOX {

    ToxCoreAV::ToxCoreAV(ToxCore& toxCore): QThread(0),
        fToxCore(toxCore), fToxAV(nullptr),
        fCallStateMap(),
        fGlobalCallState(csNone),
        fAudioInput(ToxCoreAV::defaultAudioFormat()),
        fAudioOutput(ToxCoreAV::defaultAudioFormat()),
        fAudioInputPipe(nullptr), fAudioOutputPipe(nullptr),
        fActiveCallFriendID(-1)
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
        Q_UNUSED(sampling_rate);

        if (fAudioOutputPipe == nullptr) {
            emit errorOccurred("Audio output not intialized");
            endCall(friend_id); // prevents spamming this error
            return;
        }

        qint64 byteSize = sample_count * channels * 2;
        qint64 totalWritten = 0;

        while (totalWritten < byteSize) {
            qint64 written = fAudioOutputPipe->write((char*) pcm, byteSize); // in bytes

            if (written < 0) {
                qWarning() << "Error writing to audio output: " << fAudioInputPipe->errorString();
                return;
            }

            totalWritten += written;
        }
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
        fActiveCallFriendID = -1; // keep thread running, but out of call
        start();
    }

    void ToxCoreAV::beforeToxKill()
    {
        if (fToxAV == nullptr) {
            return;
        }

        fActiveCallFriendID = -2; // stop thread
        if (!wait(DEFAULT_OOC_DELAY * 2)) { // allow 2x max delay
            qWarning() << "Audio processing thread failed to finish\n";
            terminate();
            wait();
        }

        toxav_kill(fToxAV);
        fToxAV = nullptr;
    }

    void ToxCoreAV::run()
    {
        if (fToxAV == nullptr) {
            qWarning() << "toxav_iterate called with null toxav\n";
            return;
        }

        while (fActiveCallFriendID > -2) {
            toxav_iterate(fToxAV);

            if (fActiveCallFriendID >= 0) {
                sendNextAudioFrame(fActiveCallFriendID);
            }

            QThread::msleep(toxav_iteration_interval(fToxAV));
        }
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
            fGlobalCallState = maxState;
            emit globalCallStateChanged(fGlobalCallState);

            if (fGlobalCallState == csActive) {
                startAudio();
            } else {
                stopAudio();
            }
        }
    }

    void ToxCoreAV::startAudio()
    {
        if (fAudioInputPipe != nullptr) {
            Utils::fatal("Audio input double open");
        }

        if (fAudioOutputPipe != nullptr) {
            Utils::fatal("Audio output double open");
        }

        qDebug() << "Starting audio!\n";

        fAudioInputPipe = fAudioInput.start();
        if (fAudioInput.error() != QAudio::NoError) {
            fAudioInputPipe = nullptr;
            return;
            emit errorOccurred("Error opening audio input");
        }

        fAudioOutputPipe = fAudioOutput.start();
        if (fAudioOutput.error() != QAudio::NoError) {
            fAudioOutputPipe = nullptr;
            return;
            emit errorOccurred("Error opening audio output");
        }

    }

    void ToxCoreAV::stopAudio()
    {

        if (fAudioOutputPipe == nullptr || fAudioInputPipe == nullptr) {
            return; // probably ringing -> none
        }

        qDebug() << "Stopping audio!\n";

        fAudioInput.stop(); // closes fAudioInputPipe
        fAudioOutput.stop(); // closes fAudioOutputPipe

        fAudioInputPipe = nullptr;
        fAudioOutputPipe = nullptr;
    }

    void ToxCoreAV::sendNextAudioFrame(quint32 friend_id)
    {
        const QByteArray micData = fAudioInputPipe->readAll();
        const qint16* pcm = (qint16*) micData.constData();
        quint32 count = (quint32) micData.size() / 2;

        TOXAV_ERR_SEND_FRAME error;
        // TODO: unhardcode channels and sampling rate
        toxav_audio_send_frame(fToxAV, friend_id, pcm, count, 2, 48000, &error);

        const QString errorStr = Utils::handleToxAVSendError(error);
        if (!errorStr.isEmpty()) {
            emit errorOccurred(errorStr);
        }
    }

    const QAudioFormat ToxCoreAV::defaultAudioFormat()
    {
        QAudioFormat result;

        result.setCodec("audio/pcm");
        result.setSampleType(QAudioFormat::SignedInt);
        result.setChannelCount(2); // TODO: this needs to be set according to frame data for incoming!
        result.setSampleSize(16);
        result.setSampleRate(48000);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(result)) {
            qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        }

        return result;
    }

}
