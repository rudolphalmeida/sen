//
// Bitwise and small utility functions
// Created by Rudolph on 19/3/22.
//

#ifndef SEN_SRC_UTILS_H_
#define SEN_SRC_UTILS_H_

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

using byte = uint8_t;
using word = uint16_t;
using cycles_t = uint64_t;

template <typename T>
inline bool isBitSet(T value, byte index) {
    return (value & (0b1 << index)) != 0;
}

template <typename T>
inline T setBit(T value, byte index) {
    return value | (0b1 << index);
}

template <typename T>
inline T resetBit(T value, byte index) {
    return value & ~(0b1 << index);
}

inline bool inRange(word low, word value, word high) {
    return low <= value && value <= high;
}

inline std::pair<byte, byte> splitWord(word value) {
    auto low = (byte)value;
    auto high = (byte)(value >> 8);
    return {high, low};
}

inline word joinWord(byte high, byte low) {
    return (word)high << 8 | (word)low;
}

std::vector<byte> readBinaryFile(const std::filesystem::path& path);

#endif  // SEN_SRC_UTILS_H_
