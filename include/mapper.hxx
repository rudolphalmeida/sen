#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "constants.hxx"
#include "util.hxx"

std::shared_ptr<Cartridge>
init_mapper(RomHeader header, std::vector<byte>&& prg_rom, std::vector<byte>&& chr_rom);

// Mapper 0
// Most basic with no switchable PRG ROM with 16KB and 32KB sizes
// and no CHR ROM banking and 8KB fixed size
class Nrom final: public Cartridge {
  public:
    explicit Nrom(
        const RomHeader& header,
        std::vector<byte>&& prg_rom,
        std::vector<byte>&& chr_rom
    ) :
        Cartridge(header),
        prg_rom{std::move(prg_rom)},
        chr_rom{std::move(chr_rom)} {}

    byte cpu_read([[maybe_unused]] uint64_t cpu_cycle, const word address) override {
        return prg_rom[map_cpu_addr(address)];
    }

    void cpu_write(uint64_t cpu_cycle, word address, byte data) override {}

    byte ppu_read(const word address) override {
        return chr_rom[address];
    }

    void ppu_write(word address, byte data) override {}

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

using Mmc1Register = SizedBitField<byte, 5>;

class Mmc1 final: public Cartridge {
  public:
    explicit Mmc1(
        const RomHeader& header,
        std::vector<byte>&& prg_rom,
        std::vector<byte>&& chr_rom
    ) :
        Cartridge(header),
        prg_rom(std::move(prg_rom)),
        chr_rom(std::move(chr_rom)) {
        if (header.chr_rom_size == 0x00) {  // iNES format only!! Size 0 indicates CHR-RAM is being used
            this->chr_rom = std::vector<byte>(0x2000, 0);
        }

        if (header.prg_ram_size) {
            spdlog::info("Initializing PRG RAM of size 0x2000");
            prg_ram = std::make_optional<std::vector<byte>>(0x8000U-0x6000U, 0);
        }
    }

    byte cpu_read([[maybe_unused]] uint64_t cpu_cycle, word address) override {
        if (InRange<word>(0x6000, address, 0x7FFF) && prg_ram) {
            return prg_ram->at(address - 0x6000U);
        }

        if (InRange<word>(0x8000, address, 0xFFFF)) {
            return prg_rom[map_cpu_addr(address)];
        }

        spdlog::debug("Unexpected address to MMC1::cpu_read {:#06X}", address);
        return 0x00;
    }

    void cpu_write(const uint64_t cpu_cycle, const word address, const byte data) override {
        if (InRange<word>(0x6000, address, 0x7FFF) && prg_ram) {
            prg_ram->at(address - 0x6000U) = data;
            return;
        }

        if (InRange<word>(0x8000, address, 0xFFFF)) {
            register_write(cpu_cycle, address, data);
            return;
        }

        spdlog::debug("Unexpected address to MMC1::cpu_write {:#06X}", address);
    }

    byte ppu_read(word address) override {
        if (InRange<word>(0x0000, address, 0x1FFF)) {
            return chr_rom[map_ppu_addr(address)];
        }
        spdlog::debug("Unexpected address to MMC1::ppu_read {:#06X}", address);
        return 0x00;
    }

    void ppu_write(word address, const byte data) override {
        if (InRange<word>(0x0000, address, 0x1FFF)) {
            chr_rom[map_ppu_addr(address)] = data;
            return;
        }
        spdlog::debug("Unexpected address to MMC1::ppu_write {:#06X}", address);
    }

    [[nodiscard]] Mirroring mirroring() const override {
        switch (control.value & 0b11U) {
            case 0: break;
            case 1: break;
            case 2: return Mirroring::Vertical;
            case 3: return Mirroring::Horizontal;
            default: break;
        }

        return header.hardware_mirroring;
    }

  private:
    std::vector<byte> prg_rom, chr_rom;
    std::optional<std::vector<byte>> prg_ram{};

    uint64_t last_cpu_write_cycle{};

    Mmc1Register control{0x0C}, chr_bank_0{}, chr_bank_1{}, prg_bank{0x10};
    Mmc1Register shift_reg{};

    byte shift_reg_write_cnt{0};

    void register_write(const uint64_t cpu_cycle, const word address, const byte data) {
        if ((last_cpu_write_cycle - cpu_cycle) < 2) {
            last_cpu_write_cycle = cpu_cycle;
            return;
        }

        if ((data & 0x80U) != 0x00) {
            shift_reg.value = 0x00;
            shift_reg_write_cnt = 0;

            control.value = 0x0C;
            prg_bank.value = 0x10;
        } else {
            shift_reg_write_cnt++;
            shift_reg.value = ((data & 0b1U) << 4U) | (shift_reg.value >> 1U);
            if (shift_reg_write_cnt == 5) {
                shift_reg_write_cnt = 0;
                switch ((address & 0x6000U) >> 13U) {
                    case 0b00: control.value = shift_reg.value; break;
                    case 0b01: chr_bank_0.value = shift_reg.value; break;
                    case 0b10: chr_bank_1.value = shift_reg.value; break;
                    case 0b11: prg_bank.value = shift_reg.value; break;
                    default: break;
                }
                shift_reg.value = 0x00;
            }
        }
    }

    [[nodiscard]] size_t map_cpu_addr(const word address) const {
        switch ((control.value & 0x0CU) >> 2U) {
            case 0b00:
            case 0b01:
                // Switching whole 32KB -> Ignore low bit of bank number
                return ((prg_bank.value & 0xEU) * 32768) + (address - 0x8000);
            case 0b10:
                if (InRange<word>(0x8000U, address, 0xBFFFU)) {
                    // Fixed first bank
                    return address - 0x8000U;
                }
                // Switchable 16KB mapped to 0xC000-0xFFFF
                return ((prg_bank.value & 0xFU) * 16384) + (address - 0xC000U);
            case 0b11:
                if (InRange<word>(0x8000U, address, 0xBFFFU)) {
                    // Switchable first bank
                    return ((prg_bank.value & 0xFU) * 16384) + (address - 0x8000U);
                }
                // Fixed last bank
                return ((header.prg_rom_banks - 1) * 16384) + (address - 0xC000U);
            default: break;
        }

        return address;
    }

    [[nodiscard]] size_t map_ppu_addr(const word address) const {
        if ((control.value & 0x10U) == 0x10U) {
            // 4KB individual bank mode
            if (InRange<word>(0x0000, address, 0x0FFF)) {
                return (chr_bank_0.value * 4096U) + address;
            }
            return (chr_bank_1.value * 4096U) + (address - 0x1000);
        }
        // 8KB whole bank mode
        return ((chr_bank_0.value & 0x1EU) * 8192U) + address;
    }
};
