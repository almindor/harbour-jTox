#ifndef CALL_H
#define CALL_H

#include <QThread>
#include <QAudioInput>
#include <QAudioOutput>
#include "tox/toxav.h"

namespace JTOX {

    class Call : public QThread
    {
    public:
        Call(ToxAV* toxAV, quint32 friend_id);

        // starts audio input + input handling thread
        bool init();
        // stops input/output and thread in a controlled manner
        void finish();
        // sets up audio output if not done yet and pushes frame data
        bool handleIncomingFrame(const qint16* pcm, size_t sample_count, quint8 channels, quint32 sampling_rate);

        void run() override; // handles audio input -> tox
    private:
        ToxAV* fToxAV;
        quint32 fFriendID;
        QAudioInput* fAudioInput;
        QAudioOutput* fAudioOutput;
        QIODevice* fAudioInputPipe;
        QIODevice* fAudioOutputPipe;
        bool fQuit;

        bool sendNextAudioFrame();
        bool startAudioInput();
        void stopAudioInput();
        bool startAudioOutput(int channels, int samplingRate);
        void stopAudioOutput();
        const QAudioFormat defaultAudioFormat(int channels, int samplingRate) const;
    };

}

#endif // CALL_H
