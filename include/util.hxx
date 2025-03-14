#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "constants.hxx"

template<typename T>
bool InRange(T low, T value, T high) {
    return low <= value && value <= high;
}

template<typename BackingType, size_t RegisterSize>
    requires((sizeof(BackingType) << 3) >= RegisterSize)
struct SizedBitField {
    BackingType value: RegisterSize;
};

std::vector<byte> ReadBinaryFile(const std::filesystem::path& path);