#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

namespace AudioUtils {

/**
 * Calculates Root Mean Square (RMS) volume level in dBFS (-60.0 dB to 0.0 dB).
 *
 * @param pcmData   Pointer to raw 16-bit PCM audio bytes.
 * @param sizeBytes Size of the buffer in bytes.
 */
inline float calculatePcmRmsDb(const uint8_t* pcmData, size_t sizeBytes) {
	if (!pcmData || sizeBytes == 0)
		return -60.0f;

	const int16_t* samples = reinterpret_cast<const int16_t*>(pcmData);
	size_t sampleCount = sizeBytes / sizeof(int16_t);

	if (sampleCount == 0)
		return -60.0f;

	double sumSq = 0.0;
	for (size_t i = 0; i < sampleCount; ++i) {
		// Normalize 16-bit sample to -1.0 .. 1.0 range
		double norm = static_cast<double>(samples[i]) / 32768.0;
		sumSq += norm * norm;
	}

	double rms = std::sqrt(sumSq / static_cast<double>(sampleCount));
	if (rms < 1e-6)
		return -60.0f;	// Floor value at -60 dB

	float db = static_cast<float>(20.0 * std::log10(rms));
	return std::clamp(db, -60.0f, 0.0f);
}

/**
 * Renders an ANSI-based terminal dashboard displaying hardware stats,
 * buffer queue health, and real-time VU level meters in-place.
 */
inline void renderDashboard(float micDb,
							float cleanDb,
							float speakerDb,
							size_t micBytes,
							size_t echoBytes,
							const std::string& inputDeviceName,
							const std::string& outputDeviceName) {
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

	// Move cursor to top-left (rewrites lines in-place)
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
	std::cout << " Input Device  : " << inputDeviceName.substr(0, 30)
			  << "\033[K\n";
	std::cout << " Output Device : " << outputDeviceName.substr(0, 30)
			  << "\033[K\n";
	std::cout
		<< " Format        : 16,000 Hz | 16-bit Mono | 10ms Frames\033[K\n\n";

	// Buffer Health & Latency Indicators
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
	std::cout << " Speaker Ref   : [" << buildVuBar(speakerDb) << "] "
			  << static_cast<int>(speakerDb) << " dB\033[K\n";

	std::cout.flush();
}

}  // namespace AudioUtils

#endif	// AUDIO_UTILS_H