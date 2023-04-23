#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <spdlog/spdlog.h>

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
   private:
    RomHeader header;
    std::vector<byte> prg_rom;
    std::vector<byte> chr_rom;

    std::unique_ptr<Mapper> mapper{};

    std::optional<std::vector<byte>> chr_ram = std::nullopt;

   public:
    Cartridge(RomHeader header,
              std::vector<byte> prg_rom,
              std::vector<byte> chr_rom,
              std::unique_ptr<Mapper> mapper,
              std::optional<std::vector<byte>> chr_ram = std::nullopt)
        : header{header},
          prg_rom{std::move(prg_rom)},
          chr_rom{std::move(chr_rom)},
          mapper{std::move(mapper)},
          chr_ram{std::move(chr_ram)} {}

    byte CpuRead(word addr) { return prg_rom[mapper->MapCpuAddr(addr)]; }
    void CpuWrite(word addr, byte data) {}

    byte PpuRead(word addr) { return chr_rom[addr]; }
    void PpuWrite(word addr, byte data) {}
};