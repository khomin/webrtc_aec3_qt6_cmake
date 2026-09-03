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

	AudioEngine(const AudioEngine&) = delete;
	AudioEngine& operator=(const AudioEngine&) = delete;

	bool initialize();
	void start();
	void stop();

private:
	void runProcessingLoop();

	QAudioDevice mAudioInfoIn;
	QAudioDevice mAudioInfoOut;

	std::unique_ptr<QAudioSource> mAudioSource;
	std::unique_ptr<QAudioSink> mAudioSink;

	QIODevice* mAudioInDevice{nullptr};
	QIODevice* mAudioOutDevice{nullptr};

	QThread* mWorkerThread{nullptr};
	std::atomic<bool> mRunning{false};

	std::unique_ptr<AecProcessor> mAecProcessor;

	static constexpr int SAMPLE_RATE = 16000;
	static constexpr size_t BYTES_PER_SAMPLE = sizeof(int16_t);	 // 2 bytes
	static constexpr size_t MAX_BUFFER_MS = 100;
	static constexpr size_t FRAME_SAMPLES = 160;
	static constexpr size_t FRAME_BYTES = FRAME_SAMPLES * sizeof(int16_t);

	Buffer mMicBuf{SAMPLE_RATE, BYTES_PER_SAMPLE, MAX_BUFFER_MS};
	Buffer mEchoBuf{SAMPLE_RATE, BYTES_PER_SAMPLE, MAX_BUFFER_MS};

	// Timing for 20 FPS HUD updates (50ms interval)
	std::chrono::steady_clock::time_point mLastHudRenderTime{};
	static constexpr std::chrono::milliseconds HUD_REFRESH_INTERVAL{50};
};