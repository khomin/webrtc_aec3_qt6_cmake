#ifndef BUFFER_H
#define BUFFER_H

#include <mutex>
#include <vector>

class Buffer {
public:
    void putData(std::vector<uint8_t> in) {
        std::lock_guard<std::mutex>l (_lock);
        _data.insert(_data.end(), in.begin(), in.end());
    }

    bool isEmpty() {
        std::lock_guard<std::mutex>l (_lock);
        return _data.empty();
    }

    int size() {
        std::lock_guard<std::mutex>l (_lock);
        return _data.size();
    }

    std::vector<uint8_t> pop(int size) {
        std::lock_guard<std::mutex>l (_lock);
        if(_data.size() >= size) {
            auto item = std::vector<uint8_t>(_data.begin(), _data.begin() + size);
            _data.erase(_data.begin(), _data.begin() + size);
            return item;
        }
        return std::vector<uint8_t>();
    }

    void clear() {
        std::lock_guard<std::mutex>l (_lock);
        _data.clear();
    }
private:
    std::vector<uint8_t> _data;
    std::mutex _lock;
};

#endif // BUFFER_H
