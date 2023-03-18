#pragma once

#include <cstddef>
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

enum Mirroring {
    Horizontal,
    Vertical,
    FourScreenVram,
};

struct RomHeader {
    Mirroring mirroring;

    size_t prg_rom_size;
    size_t chr_rom_size;

    bool battery_backed_ram = false;

    byte mapper_number;
};

struct ParsedRom {
    RomHeader header;
    std::vector<byte> prg_rom;
    std::vector<byte> chr_rom;

    std::optional<std::vector<byte>> chr_ram = std::nullopt;
};

ParsedRom parse_rom_file(const RomArgs& rom_args);

class Sen {
   public:
    Sen(RomArgs rom_args);
};
