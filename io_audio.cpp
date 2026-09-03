#include "io_audio.h"

#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioSink>
#include <QAudioSource>
#include <QCoreApplication>
#include <QDebug>
#include <QIODevice>
#include <QMediaDevices>
#include <QThread>
#include <iostream>

IoAudio::IoAudio() {
	mAudioEngine = std::make_unique<AudioEngine>();
	mAechProcessor = std::make_unique<AecProcessor>(SAMPLE_RATE);

	mAudioEngine->initialize();
}

IoAudio::~IoAudio() {
	std::cout << "destroyed" << std::endl;
	mDestroy = true;
	if (mAudioEngine != nullptr) {
		mAudioEngine->stop();
	}
}

void IoAudio::start() {
	mAudioEngine->start();
}