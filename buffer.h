#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

class Buffer {
public:
	/**
	 * Constructs a thread-safe audio buffer with explicit capacity bounds.
	 *
	 * @param sampleRateHz      Sample rate in Hz (e.g., 16000)
	 * @param bytesPerSample    Bytes per single sample (e.g., 2 for 16-bit Int)
	 * @param maxDurationMs     Maximum allowed duration in milliseconds before
	 * dropping stale data
	 */
	Buffer(int sampleRateHz, size_t bytesPerSample, size_t maxDurationMs)
		: _maxCapacityBytes(calculateCapacityBytes(sampleRateHz,
												   bytesPerSample,
												   maxDurationMs)) {}

	void putData(const std::vector<uint8_t>& in) {
		std::lock_guard<std::mutex> lock(_lock);

		// Append incoming bytes
		_data.insert(_data.end(), in.begin(), in.end());

		// Auto-drop oldest bytes if total size exceeds configured max capacity
		if (_data.size() > _maxCapacityBytes) {
			size_t overflow = _data.size() - _maxCapacityBytes;
			_data.erase(_data.begin(), _data.begin() + overflow);
		}
	}

	bool isEmpty() const {
		std::lock_guard<std::mutex> lock(_lock);
		return _data.empty();
	}

	size_t size() const {
		std::lock_guard<std::mutex> lock(_lock);
		return _data.size();
	}

	std::vector<uint8_t> pop(size_t sizeBytes) {
		std::lock_guard<std::mutex> lock(_lock);
		if (_data.size() >= sizeBytes) {
			std::vector<uint8_t> item(_data.begin(), _data.begin() + sizeBytes);
			_data.erase(_data.begin(), _data.begin() + sizeBytes);
			return item;
		}
		return {};
	}

	void clear() {
		std::lock_guard<std::mutex> lock(_lock);
		_data.clear();
	}

	size_t capacityBytes() const { return _maxCapacityBytes; }

private:
	static size_t calculateCapacityBytes(int sampleRateHz,
										 size_t bytesPerSample,
										 size_t durationMs) {
		// Explicit calculation: (samples/sec * bytes/sample * ms) / 1000 ms/sec
		return (static_cast<size_t>(sampleRateHz) * bytesPerSample *
				durationMs) /
			   1000;
	}

	std::vector<uint8_t> _data;
	mutable std::mutex _lock;
	size_t _maxCapacityBytes;
};

#endif	// BUFFER_H