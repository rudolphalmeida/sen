#pragma once

#include <cstdint>
#include <optional>
#include <vector>

using byte = uint8_t;
using word = uint16_t;
using sbyte = int8_t;

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};
