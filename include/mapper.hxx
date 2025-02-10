#pragma once

#include <memory>

#include "cartridge.hxx"
#include "constants.hxx"
#include "util.hxx"

std::shared_ptr<Cartridge> init_mapper(RomHeader header, const std::vector<byte>& prg_rom, const std::vector<byte>& chr_rom);

// Mapper 0
// Most basic with no switchable PRG ROM with 16KB and 32KB sizes
// and no CHR ROM banking and 8KB fixed size
class Nrom final : public Cartridge {
   public:
    explicit Nrom(const RomHeader& header, std::vector<byte> prg_rom, std::vector<byte> chr_rom)
        : Cartridge(header), prg_rom{std::move(prg_rom)}, chr_rom{std::move(chr_rom)} {}

    byte cpu_read(const word address) override {
        return prg_rom[map_cpu_addr(address)];
    }
    void cpu_write(word, byte) override {}

    byte ppu_read(const word address) override {
        return chr_rom[address];
    }

    void ppu_write(word, byte) override {}

    [[nodiscard]] std::span<const unsigned char> chr_rom_ref() const override {
        return std::span<const unsigned char, 8192>(chr_rom);
    }

    friend class Debugger;

private:
    std::vector<byte> prg_rom, chr_rom;

    [[nodiscard]] word map_cpu_addr(word address) const {
        if (InRange<word>(0x8000, address, 0xFFFF)) {
            return address % (16384 * header.prg_rom_banks);
        }

        spdlog::error("Unknown CPU address {:#06X} to NROM", address);
        return 0x0000;
    }
};
