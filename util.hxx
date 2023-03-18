#pragma once

#include <filesystem>
#include <vector>

#include "constants.hxx"

template <typename T>
inline bool inRange(T low, T value, T high) {
    return low <= value && value <= high;
}

std::vector<byte> read_binary_file(const std::filesystem::path& path);
