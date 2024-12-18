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
    size_t prg_rom_size;
    size_t chr_rom_size;
    Mirroring mirroring;
    word mapper_number;
    bool battery_backed_ram = false;
};

struct Cartridge {
   private:
    RomHeader header;
    std::vector<byte> prg_rom;
    std::vector<byte> chr_rom;

    std::unique_ptr<Mapper> mapper{};

    std::optional<std::vector<byte>> chr_ram = std::nullopt;

   public:
    Cartridge(const RomHeader& header,
              std::vector<byte> prg_rom,
              std::vector<byte> chr_rom,
              std::unique_ptr<Mapper> mapper,
              std::optional<std::vector<byte>> chr_ram = std::nullopt)
        : header{header},
          prg_rom{std::move(prg_rom)},
          chr_rom{std::move(chr_rom)},
          mapper{std::move(mapper)},
          chr_ram{std::move(chr_ram)} {}

    [[nodiscard]] byte CpuRead(const word address) const { return prg_rom[mapper->MapCpuAddr(address)]; }
    void CpuWrite(word address, byte data) {}

    [[nodiscard]] byte PpuRead(const word address) const { return chr_rom[address]; }
    void PpuWrite(word address, byte data) {}

    [[nodiscard]] Mirroring NametableMirroring() const {
        return header.mirroring;
    }

    friend class Debugger;
};