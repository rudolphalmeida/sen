#pragma once

#include <deque>
#include <filesystem>
#include <vector>

#include "constants.hxx"

template <typename T>
inline bool inRange(T low, T value, T high) {
    return low <= value && value <= high;
}

std::vector<byte> read_binary_file(const std::filesystem::path& path);

template <typename T>
class FixedSizeQueue {
   private:
    size_t max_size;

   public:
    std::deque<T> values{};
    FixedSizeQueue(size_t max_size) : max_size{max_size} {}

    void PushBack(T value) {
        if (values.size() > max_size) {
            values.pop_front();
        }

        values.push_back(value);
    }

    T PopFront() {
        auto value = values.front();
        values.pop_front();
        return value;
    }
};
