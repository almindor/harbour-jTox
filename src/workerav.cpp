#include "workerav.h"
#include "utils.h"
#include <QDebug>

namespace JTOX {

    //------------------------WorkerIterator---------------------------//

    void WorkerIterator::initTimer()
    {
        if (fTimer == nullptr) {
            fTimer = new QTimer();
            connect(fTimer, &QTimer::timeout, this, &WorkerIterator::iterate);
        }
    }

    WorkerIterator::WorkerIterator() : QObject(0),
        fTimer(nullptr)
    {
    }

    WorkerIterator::~WorkerIterator()
    {
        if (fTimer != nullptr) {
            fTimer->stop();
            delete fTimer;
            fTimer = nullptr;
        }
    }

    void WorkerIterator::start(int interval)
    {
        initTimer();
        fTimer->start(interval);
    }

    void WorkerIterator::stop()
    {
        if (fTimer != nullptr) { // stop before start or not-used timer
            fTimer->stop();
        }
    }

    //---------------------- WorkerToxAVIterator -----------------------//

    void WorkerToxAVIterator::iterate()
    {
        if (fToxAV == nullptr) {
            qWarning() << "toxav_iterate called with null toxav"; // TODO: proper error handling
            return;
        }

        toxav_iterate(fToxAV);
        fTimer->setInterval(toxav_iteration_interval(fToxAV));
    }

    WorkerToxAVIterator::WorkerToxAVIterator() : WorkerIterator()
    {

    }

    void WorkerToxAVIterator::onAudioFrameReceived(quint32 friend_id, const qint16* pcm, size_t sample_count, quint8 channels, quint32 sampling_rate) const
    {
        const QByteArray data((char*) pcm, 2 * sample_count * channels); // copy pcm to QByteArray for cross-thread signaling

        emit audioFrameReceived(friend_id, data, channels, sampling_rate); // send signal to audio worker thread
    }

    void WorkerToxAVIterator::start(void* toxAV)
    {
        fToxAV = (ToxAV*) toxAV;
        WorkerIterator::start(toxav_iteration_interval(fToxAV));
    }

    void WorkerToxAVIterator::stop()
    {
        WorkerIterator::stop();
        fToxAV = nullptr;
    }


    //--------------------------- WorkerAV -----------------------------//


    WorkerAV::WorkerAV() : WorkerIterator(),
        fToxAV(nullptr),
        fPipe(nullptr),
        fFriendID(-1)
    {

    }

    const QAudioFormat WorkerAV::makeAudioFormat(int channels, int sampleRate)
    {
        QAudioFormat result;

        // constant
        result.setCodec("audio/pcm");
        result.setSampleType(QAudioFormat::SignedInt);
        result.setSampleSize(16);
        // variable
        result.setChannelCount(channels);
        result.setSampleRate(sampleRate);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(result)) {
            qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        }

        return result;
    }

    void WorkerAV::start(void* toxAV, quint32 friend_id)
    {
        // don't start iterating here, some children don't need to
        fToxAV = (ToxAV*) toxAV;
        fFriendID = friend_id;
    }

    void WorkerAV::stop()
    {
        stopPipe(); // stop audio gracefully first
        WorkerIterator::stop();
    }

    //---------------------- WorkerAudioInput -------------------------//

    void WorkerAudioInput::iterate()
    {
        if (fAudioInput == nullptr) {
            qWarning() << "Audio input not read on sendNextAudioFrame()"; // TODO: proper error handling
            return;
        }
        // we're sending frames of sampled data in chunks of 20ms worth given the sampling rate
        // sampling rate * sampling time (ms) * channels / 1000
        constexpr int FRAME_TIME = 20; // 20 ms opus golden standard
        constexpr int MAX_FRAMES = 2; // 40 ms seems to be max, toxav docs say 60 ms but that errors out for me
        const QAudioFormat format = fAudioInput->format();
        int frameBytes = format.sampleRate() * format.channelCount() * FRAME_TIME / 1000;
        quint32 sampleCount = frameBytes / format.channelCount() / 2; // total bytes in frame / channels / sample_byte_size (16 bit)

        if (fAudioInput->bytesReady() < frameBytes) { // next time
            return;
        }

        int framesReady = fAudioInput->bytesReady() / frameBytes;
        int bytesAvailable = frameBytes * framesReady;

        const QByteArray micData = fPipe->read(bytesAvailable);
        if (micData.isEmpty() || micData.size() != bytesAvailable) {
            qWarning() << "Unexpected low/empty read on audio input: " << micData.size();
            return;
        }

        const qint16* pcm = (qint16*) micData.constData();

        while (framesReady > 0) {
            int frames = framesReady > MAX_FRAMES ? MAX_FRAMES : framesReady;

            TOXAV_ERR_SEND_FRAME error;
            toxav_audio_send_frame(fToxAV, fFriendID, pcm, sampleCount * frames, format.channelCount(), format.sampleRate(), &error);

            const QString errorStr = Utils::handleToxAVSendError(error);
            if (!errorStr.isEmpty()) {
                qWarning() << errorStr;
                return;
            }

            pcm += (sampleCount * frames); // move on
            framesReady -= frames; // we sent this many frames, subtract from ready ones
        }
    }

    QIODevice* WorkerAudioInput::startPipe(quint32 friend_id, quint8 channels, quint32 samplingRate)
    {
        Q_UNUSED(friend_id);

        fAudioInput = new QAudioInput(WorkerAV::makeAudioFormat(channels, samplingRate));

        // we're not using this internal thread/signalling scheme because it has daddy issues
//        connect(fAudioInput, &QAudioInput::notify, this, &WorkerAudioInput::iterate);
//        fAudioInput->setNotifyInterval(20); // opus default 20ms for voip

        fPipe = fAudioInput->start();
        if (fAudioInput->error() != QAudio::NoError) {
            fPipe = nullptr;
            qWarning() << "Error opening audio input"; // TODO: proper error handling
        }

        return fPipe;
    }

    void WorkerAudioInput::stopPipe()
    {
        fAudioInput->stop();
        delete fAudioInput;
        fAudioInput = nullptr;
        fPipe = nullptr;
    }

    WorkerAudioInput::WorkerAudioInput() : WorkerAV()
    {

    }

    void WorkerAudioInput::start(void* toxAV, quint32 friend_id)
    {
        WorkerAV::start(toxAV, friend_id); // set variables

        startPipe(friend_id, 1, 24000);
        WorkerIterator::start(15); // QAudioInput has thrading daddy issues so we read from it in this thread manually
    }

    //---------------------- WorkerAudioOutput -------------------------//

    void WorkerAudioOutput::iterate()
    {
        qWarning() << "Iterate called on audio output!";
    }

    QIODevice* WorkerAudioOutput::startPipe(quint32 friend_id, quint8 channels, quint32 samplingRate)
    {
        Q_UNUSED(friend_id);

        fAudioOutput = new QAudioOutput(WorkerAV::makeAudioFormat(channels, samplingRate));
        fPipe = fAudioOutput->start();
        if (fAudioOutput->error() != QAudio::NoError) {
            fPipe = nullptr;
            qWarning() << "Error opening audio output"; // TODO: proper error handling
        }

        qDebug() << "Started audio output with buffer size: " << fAudioOutput->bufferSize();
        return fPipe;
    }

    void WorkerAudioOutput::stopPipe()
    {
        fAudioOutput->stop();
        delete fAudioOutput;
        fAudioOutput = nullptr;
        fPipe = nullptr;
    }

    void WorkerAudioOutput::start(void* toxAV, quint32 friend_id)
    {
        WorkerAV::start(toxAV, friend_id); // set variables

        // NOTHING! we're waiting for signal from AV iterator worker to push the data to output in this thread
    }

    WorkerAudioOutput::WorkerAudioOutput() : WorkerAV(),
        fAudioOutput(nullptr)
    {

    }

    void WorkerAudioOutput::onAudioFrameReceived(quint32 friend_id, const QByteArray& data, quint8 channels, quint32 sampling_rate)
    {
        if (fPipe == nullptr) { // first frame, init pipe as needed
            qDebug() << "Starting audio output";
            fPipe = startPipe(friend_id, channels, sampling_rate);
        }

        qint64 byteSize = data.size();
        qint64 totalWritten = 0;

        while (totalWritten < byteSize) {
            qint64 written = fPipe->write(data); // in bytes

            if (written < 0) {
                qWarning() << "Error writing to audio output: " << fPipe->errorString(); // TODO: proper error handling
                return;
            }

            if (written == 0) {
                return; // TODO: buffer full I suppose, need to investigate handling this better
            }

            totalWritten += written;
        }
    }

}
