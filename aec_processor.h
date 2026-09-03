#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "api/echo_control.h"
#include "audio_processing/audio_buffer.h"
#include "audio_processing/high_pass_filter.h"

class AecProcessor {
public:
	explicit AecProcessor(int sampleRate = 16000);

	// Non-copyable, clean move semantics
	~AecProcessor() = default;

	void processFarEnd(const uint8_t* pcmData, size_t samplesCount);
	void processCapture(const uint8_t* nearEndPcm,
						uint8_t* outCleanPcm,
						size_t samplesCount);

private:
	std::mutex mLock;
	std::unique_ptr<webrtc::EchoControl> mEchoController;
	std::unique_ptr<webrtc::HighPassFilter> mHpFilter;

	webrtc::AudioFrame mRefFrame;
	webrtc::AudioFrame mAecFrame;
	std::unique_ptr<webrtc::AudioBuffer> mRefAudio;
	std::unique_ptr<webrtc::AudioBuffer> mAecAudio;

	int mSampleRate = 0;
};