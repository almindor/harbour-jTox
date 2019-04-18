#ifndef CALL_H
#define CALL_H

#include <QThread>
#include <QAudioInput>
#include <QAudioOutput>
#include "tox/toxav.h"

namespace JTOX {

    class Call : public QObject
    {
        Q_OBJECT
    public:
        Call();

        // starts audio input + input handling thread
        bool init(ToxAV* toxAV, quint32 friend_id);
        // stops input/output and thread in a controlled manner
        void finish();

        bool isRunning() const;

        quint32 friendID() const;
    signals:
        void initAudioOutput(quint8 channels, quint32 sampling_rate) const;
    public slots:
        // sets up audio output if not done yet and pushes frame data
        bool handleIncomingFrame(const QByteArray& data, quint8 channels, quint32 sampling_rate);
    private slots:
        // cross threaded coz QAudioInput cannot be created in non-parent thread but we need 1st frame info to do it
        void startAudioOutput(quint8 channels, quint32 samplingRate);
    private:
        ToxAV* fToxAV;
        quint32 fFriendID;
        QAudioInput* fAudioInput;
        QAudioOutput* fAudioOutput;
        QIODevice* fAudioInputPipe;
        QIODevice* fAudioOutputPipe;

        bool sendNextAudioFrame();
        bool startAudioInput();
        void stopAudioInput();
        void stopAudioOutput();
        const QAudioFormat defaultAudioFormat(int channels, int samplingRate) const;
    };

}

#endif // CALL_H
