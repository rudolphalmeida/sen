#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <vector>

#include "constants.hxx"

template<typename T>
inline bool InRange(T low, T value, T high) {
    return low <= value && value <= high;
}

template<typename BackingType, size_t RegisterSize>
struct SizedBitField {
    BackingType value: RegisterSize;
};

std::vector<byte> ReadBinaryFile(const std::filesystem::path& path);

template<typename T>
class FixedSizeQueue {
  private:
    size_t max_size;

  public:
    std::deque<T> values{};

    explicit FixedSizeQueue(size_t max_size) : max_size{max_size} {}

    [[nodiscard]] size_t Size() const {
        return values.size();
    }

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
