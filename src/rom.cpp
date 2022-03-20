//
// Created by rudolph on 20/3/22.
//

#include "rom.h"

size_t TRAINER_SIZE = 512;

/*
 * Parse a ROM header and ROM, RAM data
 * Based on https://www.nesdev.org/wiki/INES
 * */
Rom Rom::ParseRomFile(const std::filesystem::path& romFilePath) {
    auto romDataBuffer = readBinaryFile(romFilePath);
    auto romData = romDataBuffer.begin();

    if (*(romData++) != 0x4E || *(romData++) != 0x45 || *(romData++) != 0x53 ||
        *(romData++) != 0x1A) {
        spdlog::error("{} is not a valid NES ROM file. Exiting",
                      romFilePath.string());
        std::exit(-1);
    }

    auto prgRomSize = 16384 * *(romData++);
    spdlog::info("PRG ROM size: {}B", prgRomSize);

    auto chrRomSize = 8192 * *(romData++);
    spdlog::info("CHR ROM size: {}B", chrRomSize);

    auto flag6 = *romData++;
    auto flag7 = *romData++;

    if ((flag7 & 0x0C) == 0x08) {
        spdlog::error("NES2.0 format ROMs are not supported. Exiting");
        std::exit(-1);
    }

    auto prgRamSize = 8192 * *(romData++);
    prgRamSize = prgRamSize == 0 ? 8192 : prgRamSize;
    spdlog::info("PRG RAM size: {}B", prgRamSize);

    // Ignore Flags (bytes) 9-15 for iNES
    for (int i = 9; i < 16; i++, romData++)
        ;

    // Extract trainer if it exists
    std::vector<byte> trainer;
    if (isBitSet(flag6, 2)) {
        spdlog::info("Found {}B trainer in ROM", TRAINER_SIZE);
        trainer.reserve(TRAINER_SIZE);
        std::copy_n(romData, TRAINER_SIZE, std::back_inserter(trainer));
    } else {
        trainer.reserve(0);
    }

    // Extract PRG ROM
    std::vector<byte> prgRom;
    prgRom.reserve(prgRomSize);
    std::copy_n(romData, prgRomSize, std::back_inserter(prgRom));

    // Extract CHR ROM
    std::vector<byte> chrRom;
    chrRom.reserve(chrRomSize);
    std::copy_n(romData, chrRomSize, std::back_inserter(chrRom));

    return Rom{flag6, flag7, std::move(trainer), std::move(prgRom),
               std::move(chrRom)};
}
