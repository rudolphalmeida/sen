#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "mapper.hxx"
#include "sen.hxx"

Sen::Sen(RomArgs rom_args) {
    auto cartridge = ParseRomFile(rom_args);
    ppu = std::make_shared<Ppu>();
    bus = std::make_shared<Bus>(std::move(cartridge), ppu);
    cpu = Cpu(bus);
}

void Sen::Run() {
    while (true) {
        cpu.Execute();
    }
}

Cartridge ParseRomFile(const RomArgs& rom_args) {
    auto rom_iter = rom_args.rom.cbegin();

    if (*rom_iter++ != '\x4E' || *rom_iter++ != '\x45' || *rom_iter++ != '\x53' ||
        *rom_iter++ != '\x1A') {
        // ROM should begin with NES\x1A
        spdlog::error("Provided ROM is not a valid NES ROM");
        std::exit(-1);
    }

    size_t prg_rom_banks = *rom_iter++;
    size_t prg_rom_size = 16384 * prg_rom_banks;
    spdlog::debug("PRG ROM size (Bytes): {}", prg_rom_size);

    size_t chr_rom_banks = *rom_iter++;
    size_t chr_rom_size = 8192 * chr_rom_banks;
    spdlog::debug("CHR ROM size (Bytes): {}", chr_rom_size);

    auto flag_6 = *rom_iter++;
    Mirroring mirroring{};
    if (flag_6 & 0b1000) {
        // Ignore mirroring control bit if this bit is set
        mirroring = Mirroring::FourScreenVram;
    } else if (flag_6 & 0b1) {
        mirroring = Mirroring::Vertical;
    } else {
        mirroring = Mirroring::Horizontal;
    }

    bool battery_backed_ram = (flag_6 & 0b10) == 0b10;
    if (battery_backed_ram) {
        // TODO: If battery_backed_ram is True, check for RAM provided in rom_args
        spdlog::debug("Cartridge uses battery backed RAM");
    }

    auto flag_7 = *rom_iter++;
    if ((flag_7 & 0b1100) == 0b1000) {
        spdlog::error("ROM is in NES2.0 format which is not implemented");
        std::exit(-1);
    }

    byte mapper_number = (flag_7 & 0xF0) | ((flag_6 & 0xF0) >> 4);

    RomHeader header{mirroring, prg_rom_size, chr_rom_size, battery_backed_ram, mapper_number};

    // Ignore bytes 8-15 while NES2.0 is not implemented
    std::advance(rom_iter, 16 - 8);

    // Check if a 512-byte trainer is present and is so, ignore it
    if (flag_6 & 0b100) {
        std::advance(rom_iter, 512);
    }

    // Read PRG-ROM
    std::vector<byte> prg_rom;
    prg_rom.reserve(prg_rom_size);
    std::copy_n(rom_iter, prg_rom_size, std::back_inserter(prg_rom));

    // Read CHR-ROM
    std::vector<byte> chr_rom;
    chr_rom.reserve(chr_rom_size);
    std::copy_n(rom_iter, chr_rom_size, std::back_inserter(chr_rom));

    auto mapper = MapperFromInesNumber(mapper_number, prg_rom_banks, chr_rom_banks);

    return Cartridge{header, prg_rom, chr_rom, std::move(mapper)};
}
