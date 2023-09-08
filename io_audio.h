#ifndef IO_AUDIO_H
#define IO_AUDIO_H

#include <QThread>
#include <QtMultimedia/QtMultimedia>
#include <QAudioInput>
#include <QAudioOutput>
#include <QCoreApplication>
#include <mutex>
#include <memory>
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "audio_processing/audio_buffer.h"
#include "audio_processing/high_pass_filter.h"
#include "api/echo_control.h"
#include "buffer.h"

class IoAudio {
public:
    explicit IoAudio();
    ~IoAudio();

    enum class AudioMode { Audio16Khz, Audio8Khz };
    enum class StatusType { Stop, Run, Destroy };

    void startAudio();

    bool isStarted() { return m_status == StatusType::Run; }

private:
    bool initAudio(AudioMode mode = AudioMode::Audio16Khz);
    bool initFormat();

    bool initAec(int sampleRate);
    void aecPutFarEndFrame(std::vector<uint8_t> &in, int samplesCount);
    void aecProcess(std::vector<uint8_t> &in,
                    std::vector<uint8_t> &out, int samplesCount);
    void destroyAec();

    QAudioSource* m_audioSource;
    QAudioSink* m_audioSink;
    QIODevice* m_audioInDevice;
    QIODevice* m_audioOutDevice;

    QAudioDevice audio_info_in;
    QAudioDevice audio_info_out;
    AudioMode m_mode;

    QThread* m_api_thread;
    QThread* m_qobject_thread;
    QCoreApplication* m_app;

    Buffer _audioOutBuf;
    Buffer _micBuf;
    Buffer _sendToNetBuf;

    Buffer _echoBuff;
    std::mutex _aecLock;

    std::shared_ptr<webrtc::AudioBuffer> aec_audio;
    std::shared_ptr<webrtc::AudioBuffer> ref_audio;
    std::shared_ptr<webrtc::HighPassFilter> hp_filter;
    std::shared_ptr<webrtc::EchoControl> echo_controler;
    webrtc::AudioFrame ref_frame, aec_frame;

    std::atomic<StatusType> m_status;

    static constexpr int MIN_SAMPLE_SIZE = 160 * 2;
    static constexpr const char* const TAG = "IoAudio";
};

#endif // IO_AUDIO_H
