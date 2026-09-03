#include "audio_engine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "audio_utils.h"

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
	stop();
}

bool AudioEngine::initialize() {
	mAudioInfoIn = QMediaDevices::defaultAudioInput();
	mAudioInfoOut = QMediaDevices::defaultAudioOutput();

	QAudioFormat format;
	format.setSampleRate(SAMPLE_RATE);
	format.setSampleFormat(QAudioFormat::SampleFormat::Int16);
	format.setChannelCount(1);

	if (!mAudioInfoIn.isFormatSupported(format) || !mAudioInfoOut.isFormatSupported(format)) {
		std::cout << "AudioEngine: Required 16kHz Mono Int16 format not supported by hardware"
				  << std::endl;
		return false;
	}

	std::cout << "AudioEngine: Input Device  -> " << mAudioInfoIn.description().toStdString()
			  << std::endl;
	std::cout << "AudioEngine: Output Device -> " << mAudioInfoOut.description().toStdString()
			  << std::endl;

	mAudioSource = std::make_unique<QAudioSource>(mAudioInfoIn, format);
	mAudioSink = std::make_unique<QAudioSink>(mAudioInfoOut, format);

	mAecProcessor = std::make_unique<AecProcessor>(SAMPLE_RATE);

	return true;
}

void AudioEngine::start() {
	if (mRunning.exchange(true)) {
		return;
	}
	mAudioInDevice = mAudioSource->start();
	mAudioOutDevice = mAudioSink->start();

	mWorkerThread = QThread::create([this] { runProcessingLoop(); });
	mWorkerThread->start();
}

void AudioEngine::stop() {
	if (!mRunning.exchange(false)) {
		return;
	}
	if (mWorkerThread && mWorkerThread->isRunning()) {
		mWorkerThread->wait();
		delete mWorkerThread;
		mWorkerThread = nullptr;
	}
	if (mAudioSource)
		mAudioSource->stop();
	if (mAudioSink)
		mAudioSink->stop();

	mAudioInDevice = nullptr;
	mAudioOutDevice = nullptr;

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
			mAecProcessor->processCapture(rawMic.data(), cleanMic.data(), FRAME_SAMPLES);

			// ------------------------------------------------------------------
			// 4. DEMO LOOPBACK & ECHO FEED
			// ------------------------------------------------------------------
			if (mAudioSink->bytesFree() >= FRAME_BYTES) {
				mAudioOutDevice->write(reinterpret_cast<const char*>(cleanMic.data()), FRAME_BYTES);

				mEchoBuf.putData(cleanMic);
			}

			// ------------------------------------------------------------------
			// 5. RENDER HUD (Using pre-pop sampled queue sizes)
			// ------------------------------------------------------------------
			auto now = std::chrono::steady_clock::now();
			if (now - mLastHudRenderTime >= HUD_REFRESH_INTERVAL) {
				mLastHudRenderTime = now;
				float rawDb = AudioUtils::calculatePcmRmsDb(rawMic.data(), FRAME_BYTES);
				float cleanDb = AudioUtils::calculatePcmRmsDb(cleanMic.data(), FRAME_BYTES);
				float echoDb = AudioUtils::calculatePcmRmsDb(echoFrame.data(), FRAME_BYTES);
				AudioUtils::renderDashboard(rawDb, cleanDb, echoDb, currentMicQueue,
					currentEchoQueue, mAudioInfoIn.description().toStdString(),
					mAudioInfoOut.description().toStdString());
			}
		}
		QThread::msleep(2);
	}
	std::cout << "AudioEngine: Worker thread exited cleanly" << std::endl;
}