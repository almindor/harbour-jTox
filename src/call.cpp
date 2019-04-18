#include "call.h"
#include "utils.h"
#include <QDebug>

namespace JTOX {

    Call::Call() : QObject(0),
        fToxAV(nullptr),
        fFriendID(-1),
        fAudioInput(nullptr), fAudioOutput(nullptr),
        fAudioInputPipe(nullptr), fAudioOutputPipe(nullptr)
    {
        connect(this, &Call::initAudioOutput, this, &Call::startAudioOutput);
    }

    bool Call::init(ToxAV* toxAV, quint32 friend_id)
    {
        if (toxAV == nullptr) {
            qWarning() << "Attempt to init Call with null toxAV";
            return false;
        }

        fToxAV = toxAV;
        fFriendID = friend_id;

        if (!startAudioInput()) {
            return false;
        }

        return true;
    }

    void Call::finish()
    {
        stopAudioInput();
        stopAudioOutput();
    }

    bool Call::isRunning() const
    {
        return fAudioInput != nullptr || fAudioOutput != nullptr;
    }

    bool Call::handleIncomingFrame(const QByteArray& data, quint8 channels, quint32 sampling_rate)
    {
//        if (fAudioOutputPipe == nullptr) {
//            emit initAudioOutput(channels, sampling_rate);
//            return false; // need to init first, ignore the data until we're done there (should be milliseconds loss)
//        }

//        qint64 byteSize = data.size();
//        qint64 totalWritten = 0;

//        while (totalWritten < byteSize) {
//            qint64 written = fAudioOutputPipe->write(data);

//            if (written < 0) {
//                qWarning() << "Error writing to audio output: " << fAudioInputPipe->errorString();
//                return false;
//            }

//            totalWritten += written;
//        }

        return true;
    }

    quint32 Call::friendID() const
    {
        return fFriendID;
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
            qWarning() << "Audio input already initialized?";
            return false; // we're done
        }

        // TODO: detect best channels and sampling rate based on device capabilities
        int channels = 2;
        int samplingRate = 48000;

        fAudioInput = new QAudioInput(defaultAudioFormat(channels, samplingRate));

        connect(fAudioInput, &QAudioInput::notify, this, &Call::sendNextAudioFrame);

        fAudioInput->setNotifyInterval(200); // opus standard for voice
        fAudioInputPipe = fAudioInput->start();
        fAudioInput->resume();

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

    void Call::startAudioOutput(quint8 channels, quint32 samplingRate)
    {
        if (fAudioOutput != nullptr && fAudioOutputPipe != nullptr) {
            return; // we're done
        }

        qDebug() << "Starting audio output channels: " << channels << " samplingRate: " << samplingRate;

        fAudioOutput = new QAudioOutput(defaultAudioFormat(channels, samplingRate));
        fAudioOutputPipe = fAudioOutput->start();

        return;
    }

    void Call::stopAudioOutput()
    {
        if (fAudioOutput == nullptr) {
            return;
        }

        fAudioOutput->stop();
        delete fAudioOutput;

        fAudioOutput = nullptr;
        fAudioOutputPipe = nullptr;
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
