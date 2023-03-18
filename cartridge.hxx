#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "constants.hxx"
#include "mapper.hxx"

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

struct Cartridge {
    RomHeader header;
    std::vector<byte> prg_rom;
    std::vector<byte> chr_rom;

    std::unique_ptr<Mapper> mapper{};

    std::optional<std::vector<byte>> chr_ram = std::nullopt;
};
