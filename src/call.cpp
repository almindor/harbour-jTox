#include "call.h"
#include "utils.h"
#include <QDebug>

namespace JTOX {

    Call::Call(ToxAV* toxAV, quint32 friend_id) :
        fToxAV(toxAV),
        fFriendID(friend_id),
        fAudioInput(nullptr), fAudioOutput(nullptr),
        fAudioInputPipe(nullptr), fAudioOutputPipe(nullptr),
        fQuit(false)
    {

    }

    bool Call::init()
    {
        if (!startAudioInput()) {
            return false;
        }
        // output is started with incoming frame data
        start();

        return true;
    }

    void Call::finish()
    {
        fQuit = true;

        if (!wait(2000)) {
            qWarning() << "Audio input processing thread cleanup failed";
            terminate();
        }

        stopAudioInput();
        stopAudioOutput();
    }

    bool Call::handleIncomingFrame(const qint16* pcm, size_t sample_count, quint8 channels, quint32 sampling_rate)
    {
        startAudioOutput(channels, sampling_rate);

        if (fAudioOutputPipe == nullptr) {
            qWarning() << "Audio output not intialized";
            return false;
        }

        qint64 byteSize = sample_count * channels * 2;
        qint64 totalWritten = 0;

        while (totalWritten < byteSize) {
            qint64 written = fAudioOutputPipe->write((char*) pcm, byteSize); // in bytes

            if (written < 0) {
                qWarning() << "Error writing to audio output: " << fAudioInputPipe->errorString();
                return false;
            }

            totalWritten += written;
        }

        return true;
    }

    void Call::run()
    {
        while (!fQuit) {
            sendNextAudioFrame();
            msleep(20); // Opus optimal
        }
    }

    bool Call::sendNextAudioFrame()
    {
        // we're sending frames of sampled data in chunks of 20ms worth given the sampling rate
        // sampling rate * sampling time (ms) * channels / 1000
        constexpr int BYTES_REQUIRED = 48 * 20 * 2; // TODO: unhardcore sampling rate and channels, keep 20ms as optimal
        constexpr quint32 SAMPLE_COUNT = BYTES_REQUIRED / 2 / 2; // total bytes in frame / channels / sample_byte_size (16 bit)

        if (fAudioInput->bytesReady() < BYTES_REQUIRED) { // next time
            return false;
        }

        while (fAudioInput->bytesReady() >= BYTES_REQUIRED) {
            const QByteArray micData = fAudioInputPipe->read(BYTES_REQUIRED);
            if (micData.isEmpty()) {
                qWarning() << "Unexpected error read on audio input";
                return false;
            }

            const qint16* pcm = (qint16*) micData.constData();

            TOXAV_ERR_SEND_FRAME error;
            // TODO: unhardcode channels and sampling rate
            toxav_audio_send_frame(fToxAV, fFriendID, pcm, SAMPLE_COUNT, 2, 48000, &error);

            const QString errorStr = Utils::handleToxAVSendError(error);
            if (!errorStr.isEmpty()) {
                qWarning() << errorStr;
                return false;
            }
        }

        return true;
    }

    bool Call::startAudioInput()
    {
        if (fAudioInput != nullptr && fAudioInputPipe != nullptr) {
            return false; // we're done
        }

        // TODO: detect best channels and sampling rate based on device capabilities
        int channels = 2;
        int samplingRate = 48000;

        fAudioInput = new QAudioInput(defaultAudioFormat(channels, samplingRate));
        fAudioInputPipe = fAudioInput->start();

        return true;
    }

    void Call::stopAudioInput()
    {
        if (fAudioInput == nullptr) {
            return;
        }

        fAudioInput->stop();
        delete fAudioInput;

        fAudioInputPipe = nullptr;
        fAudioInput = nullptr;
    }

    bool Call::startAudioOutput(int channels, int samplingRate)
    {
        if (fAudioOutput != nullptr && fAudioOutputPipe != nullptr) {
            return false; // we're done
        }

        fAudioOutput = new QAudioOutput(defaultAudioFormat(channels, samplingRate));
        fAudioOutputPipe = fAudioOutput->start();

        return true;
    }

    void Call::stopAudioOutput()
    {
        if (fAudioInput == nullptr) {
            return;
        }

        fAudioInput->stop();
        delete fAudioInput;

        fAudioInputPipe = nullptr;
        fAudioInput = nullptr;
    }

    const QAudioFormat Call::defaultAudioFormat(int channels, int samplingRate) const
    {
        QAudioFormat result;

        // constants
        result.setCodec("audio/pcm");
        result.setSampleType(QAudioFormat::SignedInt);
        result.setSampleSize(16);
        // variables
        result.setChannelCount(channels);
        result.setSampleRate(samplingRate);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(result)) {
            qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        }

        return result;
    }

}
