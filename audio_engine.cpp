#include "audio_engine.h"

#include <QDebug>
#include <iostream>

#include <algorithm>
#include <cmath>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

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

static float calculatePcmRmsDb(const uint8_t* pcmData, size_t sizeBytes) {
	if (sizeBytes == 0)
		return -60.0f;

	const int16_t* samples = reinterpret_cast<const int16_t*>(pcmData);
	size_t sampleCount = sizeBytes / sizeof(int16_t);

	double sumSq = 0.0;
	for (size_t i = 0; i < sampleCount; ++i) {
		double norm = samples[i] / 32768.0;
		sumSq += norm * norm;
	}

	double rms = std::sqrt(sumSq / sampleCount);
	if (rms < 1e-6)
		return -60.0f;

	float db = static_cast<float>(20.0 * std::log10(rms));
	return std::clamp(db, -60.0f, 0.0f);
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
	std::vector<uint8_t> echoFrame(FRAME_BYTES);

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
			// ------------------------------------------------------------------
			// 1. SAMPLE METRICS BEFORE POPPING FOR ACCURATE DASHBOARD QUEUES
			// ------------------------------------------------------------------
			size_t currentMicQueue = mMicBuf.size();
			size_t currentEchoQueue = mEchoBuf.size();

			// ------------------------------------------------------------------
			// 2. FETCH AUDIO FRAMES
			// ------------------------------------------------------------------
			auto rawMic = mMicBuf.pop(FRAME_BYTES);

			if (mEchoBuf.size() >= FRAME_BYTES) {
				echoFrame = mEchoBuf.pop(FRAME_BYTES);
			} else {
				std::fill(echoFrame.begin(), echoFrame.end(), 0);
			}

			// ------------------------------------------------------------------
			// 3. WEBRTC AEC3 PROCESSING
			// ------------------------------------------------------------------
			// Pass far-end speaker reference first
			mAecProcessor->processFarEnd(echoFrame.data(), FRAME_SAMPLES);

			// Pass near-end mic capture second
			mAecProcessor->processCapture(rawMic.data(), cleanMic.data(),
										  FRAME_SAMPLES);

			// ------------------------------------------------------------------
			// 4. DEMO LOOPBACK & ECHO FEED
			// ------------------------------------------------------------------
			if (mAudioSink->bytesFree() >= FRAME_BYTES) {
				mAudioOutDevice->write(
					reinterpret_cast<const char*>(cleanMic.data()),
					FRAME_BYTES);

				mEchoBuf.putData(cleanMic);
			}

			// ------------------------------------------------------------------
			// 5. RENDER HUD (Using pre-pop sampled queue sizes)
			// ------------------------------------------------------------------
			auto now = std::chrono::steady_clock::now();
			if (now - mLastHudRenderTime >= HUD_REFRESH_INTERVAL) {
				mLastHudRenderTime = now;

				float rawDb = calculatePcmRmsDb(rawMic.data(), FRAME_BYTES);
				float cleanDb = calculatePcmRmsDb(cleanMic.data(), FRAME_BYTES);
				float echoDb = calculatePcmRmsDb(echoFrame.data(), FRAME_BYTES);

				// Pass currentMicQueue and currentEchoQueue to renderDashboard!
				renderDashboard(rawDb, cleanDb, echoDb, currentMicQueue,
								currentEchoQueue);
			}
		}

		// Pace loop at ~2ms intervals to prevent 100% CPU spinning
		QThread::msleep(2);
	}

	std::cout << "AudioEngine: Worker thread exited cleanly." << std::endl;
}

void AudioEngine::renderDashboard(float micDb,
								  float cleanDb,
								  float speakerDb,
								  size_t micBytes,
								  size_t echoBytes) {
	// Colorized VU meter bar helper
	auto buildVuBar = [](float db, int width = 20) {
		float norm = (db + 60.0f) / 60.0f;	// Map -60dB..0dB -> 0.0..1.0
		norm = std::clamp(norm, 0.0f, 1.0f);
		int filled = static_cast<int>(norm * width);

		std::string bar;
		for (int i = 0; i < width; ++i) {
			if (i < filled) {
				if (i < width * 0.7)
					bar += "\033[32m█\033[0m";	// Green
				else if (i < width * 0.9)
					bar += "\033[33m█\033[0m";	// Yellow
				else
					bar += "\033[31m█\033[0m";	// Red
			} else {
				bar += "░";
			}
		}
		return bar;
	};

	// Move cursor to top-left (rewrites lines in-place, preventing flicker)
	std::cout << "\033[H";

	// Header
	std::cout << "\033[1;36m==================================================="
				 "=\033[0m\033[K\n";
	std::cout << "\033[1;37m   AEC3 Audio Engine Terminal Dashboard            "
				 " \033[0m\033[K\n";
	std::cout << "\033[1;36m==================================================="
				 "=\033[0m\033[K\n\n";

	// Hardware Stats
	std::cout << "\033[1m[ HARDWARE & FORMAT ]\033[0m\033[K\n";
	std::cout << " Input Device  : "
			  << mAudioInfoIn.description().toStdString().substr(0, 30)
			  << "\033[K\n";
	std::cout << " Output Device : "
			  << mAudioInfoOut.description().toStdString().substr(0, 30)
			  << "\033[K\n";
	std::cout
		<< " Format        : 16,000 Hz | 16-bit Mono | 10ms Frames\033[K\n\n";

	// Buffer Health & Latency Indicators (using passed pre-pop values)
	std::cout << "\033[1m[ BUFFER QUEUES ]\033[0m\033[K\n";
	std::cout << " Mic Queue     : " << micBytes << " B ";
	if (micBytes > 1280)
		std::cout << "\033[1;31m[OVERFLOW WARNING]\033[0m";
	else
		std::cout << "\033[32m[OK]\033[0m";
	std::cout << "\033[K\n";

	std::cout << " Echo Queue    : " << echoBytes << " B ";
	if (echoBytes > 0)
		std::cout << "\033[32m[ACTIVE]\033[0m";
	else
		std::cout << "\033[33m[SILENCE / STARVED]\033[0m";
	std::cout << "\033[K\n\n";

	// Real-Time Audio Meters
	std::cout << "\033[1m[ LIVE AUDIO LEVELS ]\033[0m\033[K\n";
	std::cout << " Raw Mic In    : [" << buildVuBar(micDb) << "] "
			  << static_cast<int>(micDb) << " dB\033[K\n";
	std::cout << " AEC3 Cleaned  : [" << buildVuBar(cleanDb) << "] "
			  << static_cast<int>(cleanDb) << " dB\033[K\n";
	std::cout << " Speaker Reference: [" << buildVuBar(speakerDb) << "] "
			  << static_cast<int>(speakerDb) << " dB\033[K\n";

	std::cout.flush();
}