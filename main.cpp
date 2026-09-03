#include "io_audio.h"

#include <QThread>
#include <csignal>
#include <iostream>

namespace {
std::atomic<bool> gKeepRunning{true};

void signalHandler(int signal) {
	if (signal == SIGINT || signal == SIGTERM) {
		gKeepRunning.store(false);
	}
}
}  // namespace

int main(int argc, char* argv[]) {
	std::cout << "Starting Audio System..." << std::endl;

	// 1. Register OS termination signals (Ctrl+C and kill/SIGTERM)
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	// 2. Initialize Audio Stack via RAII
	auto audio = std::make_unique<IoAudio>();
	audio->start();

	std::cout << "Audio running. Press Ctrl+C to exit." << std::endl;

	// 3. Clean main wait loop — zero CPU load, listens directly for
	// gKeepRunning
	while (gKeepRunning.load()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	// 4. Graceful Cleanup
	std::cout << "\nCaught signal. Shutting down gracefully..." << std::endl;

	// Unique pointer reset invokes ~IoAudio() -> AudioEngine::stop()
	audio.reset();

	std::cout << "Audio Engine stopped cleanly. Exiting." << std::endl;
	return 0;
}
