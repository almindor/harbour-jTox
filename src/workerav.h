#ifndef WORKERAV_H
#define WORKERAV_H

#include <tox/toxav.h>
#include <QObject>
#include <QTimer>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioFormat>

namespace JTOX {

    // generic ToxAV* worker iterator
    class WorkerIterator : public QObject
    {
        Q_OBJECT
    protected:
        QTimer* fTimer;

        virtual void iterate() = 0;
        void initTimer();
    public:
        explicit WorkerIterator();
        virtual ~WorkerIterator();
    public slots:
        virtual void start(int interval);
        virtual void stop();
    };

    class WorkerToxAVIterator : public WorkerIterator
    {
        Q_OBJECT
        ToxAV* fToxAV;

        virtual void iterate() override;
    signals:
        // signal for c-callback connected to toxav_iterate, we need to pass the data along to the audio output thread worker
        void audioFrameReceived(quint32 friend_id, const QByteArray& data, quint8 channels, quint32 samplingRate) const;
    public:
        explicit WorkerToxAVIterator();

        // c-callback connected to toxav_iterate, we need to pass the data along to the audio output thread worker
        void onAudioFrameReceived(quint32 friend_id, const qint16* pcm, size_t sample_count, quint8 channels, quint32 sampling_rate) const;
    public slots:
        virtual void start(void* toxAV);
        virtual void stop() override;
    };

    class WorkerAV : public WorkerIterator
    {
        Q_OBJECT
    protected:
        ToxAV* fToxAV;
        QIODevice* fPipe;
        quint32 fFriendID;

        virtual QIODevice* startPipe(quint32 friend_id, quint8 channels, quint32 samplingRate) = 0;
        virtual void stopPipe() = 0;
    public:
        explicit WorkerAV();

        // used by all Audio workers
        static const QAudioFormat makeAudioFormat(int channels, int sampleRate);
    public slots:
        virtual void start(void* toxAV, quint32 friend_id);
        virtual void stop() override;
    };

    class WorkerAudioInput : public WorkerAV
    {
        Q_OBJECT
        QAudioInput* fAudioInput;
    protected:
        virtual void iterate() override;
        virtual QIODevice* startPipe(quint32 friend_id, quint8 channels, quint32 samplingRate) override;
        virtual void stopPipe() override;
    public:
        explicit WorkerAudioInput();
    public slots:
        virtual void start(void* toxAV, quint32 friend_id) override;
    };

    class WorkerAudioOutput : public WorkerAV
    {
        Q_OBJECT
        QAudioOutput* fAudioOutput;
    protected:
        virtual void iterate() override;
        virtual QIODevice* startPipe(quint32 friend_id, quint8 channels, quint32 samplingRate) override;
        virtual void stopPipe() override;
    public:
        explicit WorkerAudioOutput();
    public slots:
        virtual void start(void* toxAV, quint32 friend_id) override;
        // signalled from main toxAV iterator worker
        void onAudioFrameReceived(quint32 friend_id, const QByteArray& data, quint8 channels, quint32 sampling_rate);
    };
}

#endif // WORKERAV_H
