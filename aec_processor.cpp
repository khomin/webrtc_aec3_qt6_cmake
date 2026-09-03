#include "aec_processor.h"

#include "api/echo_canceller3_config.h"
#include "api/echo_canceller3_factory.h"
#include "audio_processing/audio_buffer.h"
#include "audio_processing/high_pass_filter.h"
#include "audio_processing/include/audio_processing.h"

#include <algorithm>
#include <iostream>

AecProcessor::AecProcessor(int sampleRate) {
	mSampleRate = sampleRate;

	std::lock_guard echo_mux(mLock);
	int channels = 1;

	webrtc::EchoCanceller3Config aec_config;
	aec_config.filter.export_linear_aec_output = false;

	webrtc::EchoCanceller3Factory aec_factory = webrtc::EchoCanceller3Factory(aec_config);
	mEchoController = aec_factory.Create(sampleRate, channels, channels);
	mHpFilter = std::make_unique<webrtc::HighPassFilter>(sampleRate, channels);

	webrtc::StreamConfig config = webrtc::StreamConfig(sampleRate, channels, true);

	mRefAudio = std::make_unique<webrtc::AudioBuffer>(config.sample_rate_hz(),
		config.num_channels(), config.sample_rate_hz(), config.num_channels(),
		config.sample_rate_hz(), config.num_channels());
	mAecAudio = std::make_unique<webrtc::AudioBuffer>(config.sample_rate_hz(),
		config.num_channels(), config.sample_rate_hz(), config.num_channels(),
		config.sample_rate_hz(), config.num_channels());
}

void AecProcessor::processFarEnd(const uint8_t* pcmData, size_t samplesCount) {
	std::lock_guard echo_mux(mLock);
	int sampleRate = mSampleRate;
	size_t frameSamples = static_cast<size_t>(sampleRate / 100);
	if (frameSamples == 0 || samplesCount == 0) {
		return;
	}

	size_t nCount = samplesCount / frameSamples;
	const int16_t* pcm16 = reinterpret_cast<const int16_t*>(pcmData);
	int channels = 1;

	for (size_t i = 0; i < nCount; i++) {
		const int16_t* framePtr = pcm16 + (i * frameSamples);

		mRefFrame.UpdateFrame(0, framePtr, frameSamples, sampleRate,
			webrtc::AudioFrame::kNormalSpeech, webrtc::AudioFrame::kVadActive, channels);
		mRefAudio->CopyFrom(&mRefFrame);
		mHpFilter->Process(mRefAudio.get(), false);
		mEchoController->AnalyzeRender(mRefAudio.get());
	}
}

void AecProcessor::processCapture(const uint8_t* nearEndPcm,
	uint8_t* outCleanPcm,
	size_t samplesCount) {
	std::lock_guard echo_mux(mLock);
	size_t frameSamples = static_cast<size_t>(mSampleRate / 100);
	if (frameSamples == 0 || samplesCount == 0) {
		return;
	}
	size_t nCount = samplesCount / frameSamples;
	const int16_t* inPcm16 = reinterpret_cast<const int16_t*>(nearEndPcm);
	int16_t* outPcm16 = reinterpret_cast<int16_t*>(outCleanPcm);
	int channels = 1;

	for (size_t i = 0; i < nCount; i++) {
		const int16_t* inFramePtr = inPcm16 + (i * frameSamples);
		int16_t* outFramePtr = outPcm16 + (i * frameSamples);

		mAecFrame.UpdateFrame(0, inFramePtr, frameSamples, mSampleRate,
			webrtc::AudioFrame::kNormalSpeech, webrtc::AudioFrame::kVadActive, channels);
		mAecAudio->CopyFrom(&mAecFrame);

		mHpFilter->Process(mAecAudio.get(), false);
		mEchoController->AnalyzeCapture(mAecAudio.get());
		mEchoController->ProcessCapture(mAecAudio.get(), true);

		mAecAudio->CopyTo(&mAecFrame);
		memcpy(outFramePtr, mAecFrame.data(), frameSamples * sizeof(int16_t));
	}
}