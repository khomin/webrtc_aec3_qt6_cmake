#include "audio_engine.h"

#include <QDebug>
#include <iostream>

#include <algorithm>
#include <cmath>

// Calculates RMS level in dB (-60.0 dB to 0.0 dB)
float calculateFrameRmsDb(const int16_t* pcmData, size_t sampleCount) {
	if (sampleCount == 0)
		return -60.0f;

	double sumSq = 0.0;
	for (size_t i = 0; i < sampleCount; ++i) {
		double sample =
			static_cast<double>(pcmData[i]) / 32768.0;	// Normalize -1.0 to 1.0
		sumSq += sample * sample;
	}

	double rms = std::sqrt(sumSq / sampleCount);
	if (rms < 1e-6)
		return -60.0f;	// Floor value

	float db = static_cast<float>(20.0 * std::log10(rms));
	return std::clamp(db, -60.0f, 0.0f);
}

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

void drawConsoleAudioMeter(float rmsLevelDb) {
	// Convert dB level (-60dB to 0dB) into a bar length (0 to 30 characters)
	constexpr int BAR_WIDTH = 30;

	// Map -60dB -> 0.0, 0dB -> 1.0
	float normalized = (rmsLevelDb + 60.0f) / 60.0f;
	normalized = std::clamp(normalized, 0.0f, 1.0f);

	int numChars = static_cast<int>(normalized * BAR_WIDTH);

	std::string meterBar(numChars, '#');
	std::string emptyBar(BAR_WIDTH - numChars, ' ');

	// \r returns cursor to start of line, \033[K clears to end of line
	std::cout << "\rMic Level: [" << meterBar << emptyBar << "] "
			  << static_cast<int>(rmsLevelDb) << " dB" << std::flush;
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
	stop();	 // Guarantee thread is dead before member destruction
}

bool AudioEngine::initialize() {
	mAudioInfoIn = QMediaDevices::defaultAudioInput();
	mAudioInfoOut = QMediaDevices::defaultAudioOutput();

	QAudioFormat format;
	format.setSampleRate(SAMPLE_RATE);
	format.setSampleFormat(QAudioFormat::SampleFormat::Int16);
	format.setChannelCount(1);

	if (!mAudioInfoIn.isFormatSupported(format) ||
		!mAudioInfoOut.isFormatSupported(format)) {
		qWarning(
			"AudioEngine: Required 16kHz Mono Int16 format not supported by "
			"hardware.");
		return false;
	}

	qDebug("AudioEngine: Input Device  -> %s",
		   qPrintable(mAudioInfoIn.description()));
	qDebug("AudioEngine: Output Device -> %s",
		   qPrintable(mAudioInfoOut.description()));

	// Instantiate hardware drivers
	mAudioSource = std::make_unique<QAudioSource>(mAudioInfoIn, format);
	mAudioSink = std::make_unique<QAudioSink>(mAudioInfoOut, format);

	// Initialize WebRTC AEC3 wrapper
	mAecProcessor = std::make_unique<AecProcessor>(SAMPLE_RATE);

	return true;
}

void AudioEngine::start() {
	if (mRunning.exchange(true))
		return;	 // Prevent double-start

	// Start hardware streams on main thread (returns internal QIODEevice
	// pointers)
	mAudioInDevice = mAudioSource->start();
	mAudioOutDevice = mAudioSink->start();

	// Launch background worker thread
	mWorkerThread = QThread::create([this] { runProcessingLoop(); });

	mWorkerThread->start();
}

void AudioEngine::stop() {
	// Atomically set flag to false
	if (!mRunning.exchange(false))
		return;

	// 1. Wait deterministically for the worker thread to exit
	if (mWorkerThread && mWorkerThread->isRunning()) {
		mWorkerThread->wait();	// Pure blocking wait - NO msleep polling!
		delete mWorkerThread;
		mWorkerThread = nullptr;
	}

	// 2. Stop hardware audio streams safely
	if (mAudioSource)
		mAudioSource->stop();
	if (mAudioSink)
		mAudioSink->stop();

	mAudioInDevice = nullptr;
	mAudioOutDevice = nullptr;

	// 3. Clear buffers
	mMicBuf.clear();
	mEchoBuf.clear();
}

void AudioEngine::runProcessingLoop() {
	std::vector<uint8_t> cleanMic(FRAME_BYTES);

	int meterDecimation = 0;

	while (mRunning) {
		if (!mAudioInDevice || !mAudioOutDevice) {
			QThread::msleep(5);
			continue;
		}

		// ------------------------------------------------------------------
		// 1. READ MICROPHONE (Near-End)
		// ------------------------------------------------------------------
		if (mAudioInDevice->bytesAvailable() > 0) {
			QByteArray raw = mAudioInDevice->readAll();
			auto data = std::vector<uint8_t>(raw.size());
			memcpy(data.data(), raw.data(), raw.size());
			mMicBuf.putData(data);
		}

		// ------------------------------------------------------------------
		// 2. PROCESS IN STRICT 10ms (320-BYTE) STEPS
		// ------------------------------------------------------------------
		while (mMicBuf.size() >= FRAME_BYTES) {
			// Pop 10ms of raw mic input
			auto rawMic = mMicBuf.pop(FRAME_BYTES);

			// Pop 10ms of far-end speaker reference if available
			if (mEchoBuf.size() >= FRAME_BYTES) {
				auto echoFrame = mEchoBuf.pop(FRAME_BYTES);
				mAecProcessor->processFarEnd(echoFrame.data(), FRAME_SAMPLES);
			}

			// Run WebRTC AEC3
			mAecProcessor->processCapture(rawMic.data(), cleanMic.data(),
										  FRAME_SAMPLES);

			if (++meterDecimation >= 5) {
				meterDecimation = 0;

				// Calculate RMS on the clean audio frame
				const int16_t* pcmPtr =
					reinterpret_cast<const int16_t*>(cleanMic.data());
				float currentDb = calculateFrameRmsDb(pcmPtr, FRAME_SAMPLES);

				// Render meter inline
				drawConsoleAudioMeter(currentDb);
			}

			// --------------------------------------------------------------
			// DEMO LOOPBACK: Write clean audio directly to hardware speaker
			// (Only write if hardware buffer has room)
			// --------------------------------------------------------------
			if (mAudioSink->bytesFree() >= FRAME_BYTES) {
				mAudioOutDevice->write(
					reinterpret_cast<const char*>(cleanMic.data()),
					FRAME_BYTES);
			}
		}

		// Pace loop at ~2ms intervals to prevent 100% CPU spinning
		QThread::msleep(2);
	}

	std::cout << "AudioEngine: Worker thread exited cleanly." << std::endl;
}