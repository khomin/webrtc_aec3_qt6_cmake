#ifndef IO_AUDIO_H
#define IO_AUDIO_H

#include <QAudioDevice>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

// #include "api/echo_control.h"
// #include "audio_processing/audio_buffer.h"
// #include "audio_processing/high_pass_filter.h"
#include "aec_processor.h"
#include "audio_engine.h"
#include "buffer.h"

class QCoreApplication;
class QAudioSource;
class QIODevice;
class QAudioSink;
class QThread;
class QAudioInput;
class QAudioOutput;

class IoAudio {
public:
	explicit IoAudio();
	~IoAudio();

	void start();

private:
	std::unique_ptr<AudioEngine> mAudioEngine{};
	std::unique_ptr<AecProcessor> mAechProcessor{};

	std::atomic<bool> mDestroy{false};

	static constexpr int MIN_SAMPLE_SIZE = 160 * 2;
	static constexpr int SAMPLE_RATE = 16000;
	static constexpr const char* const TAG = "IoAudio";
};

#endif	// IO_AUDIO_H
