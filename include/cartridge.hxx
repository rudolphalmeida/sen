#pragma once

#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>

#include "constants.hxx"

enum Mirroring {
    Horizontal,
    Vertical,
    FourScreenVram,
};

struct RomHeader {
    size_t prg_rom_size;
    size_t prg_rom_banks;

    size_t chr_rom_size;
    size_t chr_rom_banks;

    Mirroring hardware_mirroring;
    word mapper_number;
    bool battery_backed_ram = false;
};

class Cartridge {
  public:
    RomHeader header;

    explicit Cartridge(const RomHeader& header) : header{header} {}

    virtual byte cpu_read(uint64_t cpu_cycle, word address) = 0;
    virtual void cpu_write(uint64_t cpu_cycle, word address, byte data) = 0;

    virtual byte ppu_read(word address) = 0;
    virtual void ppu_write(word address, byte data) = 0;

    [[nodiscard]] virtual Mirroring mirroring() const {
        return header.hardware_mirroring;
    }

    virtual ~Cartridge() = default;

    friend class Debugger;
};