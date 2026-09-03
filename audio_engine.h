#pragma once

#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioSink>
#include <QAudioSource>
#include <QMediaDevices>
#include <QThread>

#include <atomic>
#include <memory>
#include <vector>

#include "aec_processor.h"
#include "buffer.h"

class AudioEngine {
public:
	AudioEngine();
	~AudioEngine();

	// Non-copyable
	AudioEngine(const AudioEngine&) = delete;
	AudioEngine& operator=(const AudioEngine&) = delete;

	// Lifecycle Management
	bool initialize();
	void start();
	void stop();

private:
	void runProcessingLoop();

	// Hardware Audio Objects (Qt)
	QAudioDevice mAudioInfoIn;
	QAudioDevice mAudioInfoOut;

	std::unique_ptr<QAudioSource> mAudioSource;
	std::unique_ptr<QAudioSink> mAudioSink;

	QIODevice* mAudioInDevice{nullptr};	  // Owned by mAudioSource
	QIODevice* mAudioOutDevice{nullptr};  // Owned by mAudioSink

	// Threading & Execution State
	QThread* mWorkerThread{nullptr};
	std::atomic<bool> mRunning{false};

	// AEC Domain Processor (Pure C++)
	std::unique_ptr<AecProcessor> mAecProcessor;

	// Real-time Audio Buffers
	// LockFreeRingBuffer mMicBuf{3200};	// ~100ms capacity max
	// LockFreeRingBuffer mEchoBuf{3200};	// ~100ms capacity max

	// static constexpr members exist BEFORE any instance member is initialized
	static constexpr int SAMPLE_RATE = 16000;
	static constexpr size_t BYTES_PER_SAMPLE = sizeof(int16_t);	 // 2 bytes
	static constexpr size_t MAX_BUFFER_MS = 100;

	// static constexpr int SAMPLE_RATE = 16000;
	static constexpr size_t FRAME_SAMPLES = 160;  // 10ms at 16kHz
	static constexpr size_t FRAME_BYTES = FRAME_SAMPLES * sizeof(int16_t);

	// Fully valid in C++11 and later:
	Buffer mMicBuf{SAMPLE_RATE, BYTES_PER_SAMPLE, MAX_BUFFER_MS};
	Buffer mEchoBuf{SAMPLE_RATE, BYTES_PER_SAMPLE, MAX_BUFFER_MS};

	void renderDashboard(float micDb,
						 float cleanDb,
						 float speakerDb,
						 size_t micBytes,
						 size_t echoBytes);

	// Timing for 20 FPS HUD updates (50ms interval)
	std::chrono::steady_clock::time_point mLastHudRenderTime{};
	static constexpr std::chrono::milliseconds HUD_REFRESH_INTERVAL{50};
};