#include "aec_processor.h"

#include "api/echo_canceller3_config.h"
#include "api/echo_canceller3_factory.h"
#include "audio_processing/audio_buffer.h"
#include "audio_processing/high_pass_filter.h"
#include "audio_processing/include/audio_processing.h"

#include <algorithm>

AecProcessor::AecProcessor(int sampleRate) {
	mSampleRate = sampleRate;

	std::lock_guard echo_mux(mLock);
	int channels = 1;

	webrtc::EchoCanceller3Config aec_config;
	aec_config.filter.export_linear_aec_output = false;

	webrtc::EchoCanceller3Factory aec_factory =
		webrtc::EchoCanceller3Factory(aec_config);
	mEchoController = aec_factory.Create(sampleRate, channels, channels);
	mHpFilter = std::make_unique<webrtc::HighPassFilter>(sampleRate, channels);

	webrtc::StreamConfig config =
		webrtc::StreamConfig(sampleRate, channels, true);

	mRefAudio = std::make_unique<webrtc::AudioBuffer>(
		config.sample_rate_hz(), config.num_channels(), config.sample_rate_hz(),
		config.num_channels(), config.sample_rate_hz(), config.num_channels());
	mAecAudio = std::make_unique<webrtc::AudioBuffer>(
		config.sample_rate_hz(), config.num_channels(), config.sample_rate_hz(),
		config.num_channels(), config.sample_rate_hz(), config.num_channels());
}

void AecProcessor::processFarEnd(const uint8_t* pcmData, size_t samplesCount) {
	std::lock_guard echo_mux(mLock);
	int sampleRate = mSampleRate;
	size_t samples = std::min(samplesCount, (size_t)(sampleRate / 100));
	if (samples == 0) {
		return;
	}
	size_t nCount = (samplesCount / samples);
	int channels = 1;

	for (size_t i = 0; i < nCount; i++) {
		mRefFrame.UpdateFrame(0, (int16_t*)pcmData, samples, sampleRate,
							  webrtc::AudioFrame::kNormalSpeech,
							  webrtc::AudioFrame::kVadActive, channels);
		mRefAudio->CopyFrom(&mRefFrame);
		mHpFilter->Process(mRefAudio.get(), false);
		mEchoController->AnalyzeRender(mRefAudio.get());
	}
}

void AecProcessor::processCapture(const uint8_t* nearEndPcm,
								  uint8_t* outCleanPcm,
								  size_t samplesCount) {
	std::lock_guard echo_mux(mLock);
	size_t samples = std::min(samplesCount, (size_t)(mSampleRate / 100));
	if (samples == 0) {
		return;
	}
	size_t nCount = (samplesCount / samples);
	int channels = 1;

	for (size_t i = 0; i < nCount; i++) {
		mAecFrame.UpdateFrame(0, (int16_t*)nearEndPcm, samples, mSampleRate,
							  webrtc::AudioFrame::kNormalSpeech,
							  webrtc::AudioFrame::kVadActive, channels);
		mAecAudio->CopyFrom(&mAecFrame);
		mHpFilter->Process(mAecAudio.get(), false);
		mEchoController->AnalyzeCapture(mAecAudio.get());
		mEchoController->ProcessCapture(mAecAudio.get(), true);
		mAecAudio->CopyTo(&mAecFrame);
		memcpy(outCleanPcm, mAecFrame.data(), samples * 2);
	}
}


// void IoAudio::aecPutFarEndFrame(std::vector<uint8_t>& in, int samplesCount) {
// 	std::lock_guard echo_mux(mAecLock);
// 	int sampleRate = SAMPLE_RATE;
// 	size_t samples = std::min(samplesCount, sampleRate / 100);
// 	if (samples == 0)
// 		return;
// 	size_t nCount = (samplesCount / samples);
// 	int channels = 1;

// 	for (size_t i = 0; i < nCount; i++) {
// 		mRefFrame.UpdateFrame(0, (int16_t*)in.data(), samples, sampleRate,
// 							  webrtc::AudioFrame::kNormalSpeech,
// 							  webrtc::AudioFrame::kVadActive, channels);
// 		mRefAudio->CopyFrom(&mRefFrame);
// 		mHpFilter->Process(mRefAudio.get(), false);
// 		mEchoController->AnalyzeRender(mRefAudio.get());
// 	}
// }

// void IoAudio::aecProcess(std::vector<uint8_t>& in,
// 						 std::vector<uint8_t>& out,
// 						 int samplesCount) {
// 	std::lock_guard echo_mux(mAecLock);
// 	int sampleRate = SAMPLE_RATE;
// 	size_t samples = std::min(samplesCount, sampleRate / 100);
// 	if (samples == 0)
// 		return;
// 	size_t nCount = (samplesCount / samples);
// 	int channels = 1;

// 	for (size_t i = 0; i < nCount; i++) {
// 		mAecFrame.UpdateFrame(0, (int16_t*)in.data(), samples, sampleRate,
// 							  webrtc::AudioFrame::kNormalSpeech,
// 							  webrtc::AudioFrame::kVadActive, channels);
// 		mAecAudio->CopyFrom(&mAecFrame);
// 		mHpFilter->Process(mAecAudio.get(), false);
// 		mEchoController->AnalyzeCapture(mAecAudio.get());
// 		mEchoController->ProcessCapture(mAecAudio.get(), true);
// 		mAecAudio->CopyTo(&mAecFrame);
// 		memcpy(out.data(), mAecFrame.data(), samples * 2);
// 	}
// }
