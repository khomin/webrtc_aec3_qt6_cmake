#include <QThread>
#include <QCoreApplication>
#include <csignal>
#include <iostream>

#include "audio_engine.h"

namespace {
std::atomic<bool> gKeepRunning{true};

void signalHandler(int signal) {
	if (signal == SIGINT || signal == SIGTERM) {
		gKeepRunning.store(false);
	}
}
}  // namespace

int main(int argc, char* argv[]) {
	QCoreApplication app(argc, argv);
	std::cout << "Starting ..." << std::endl;

	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	auto engine = std::make_unique<AudioEngine>();
	if (!engine->initialize()) {
		return -1;
	}
	engine->start();

	std::cout << "Running. Press Ctrl+C to exit." << std::endl;

	while (gKeepRunning.load()) {
		QCoreApplication::processEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	engine.reset();

	return 0;
}
