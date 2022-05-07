//
// Created by rudolph on 20/3/22.
//

#include "cartridge.h"

std::unique_ptr<Mapper> buildMapperFromRom(const Rom& rom);

Cartridge::Cartridge(Rom&& rom)
    : Cartridge(std::move(rom), buildMapperFromRom(rom)) {}

byte Cartridge::CpuRead(word address) {
    return mapper->Read(rom, address);
}

void Cartridge::CpuWrite(word address, byte data) {
    mapper->Write(rom, address, data);
}

std::unique_ptr<Mapper> buildMapperFromRom(const Rom& rom) {
    switch (rom.MapperNumber()) {
        case 0x00:
            spdlog::info("Loading Mapper000 as cartridge mapper");
            return std::make_unique<Mapper000>();
        default:
            spdlog::error("Mapper ID {:#4X} not implemented. Exiting",
                          rom.MapperNumber());
            std::exit(-1);
    }
}

byte Mapper000::Read(const Rom& rom, word address) const {
    if (inRange(0x8000, address, 0xFFFF)) {
        return rom.PrgRom().at(address % rom.PrgRom().size());
    }

    // This should not happen. Check for bug in MMU and Cartridge
    spdlog::error("Invalid read address for Mapper 000: {:#6X}", address);
    return 0xFF;
}

void Mapper000::Write(Rom& rom, word address, byte data) {
    if (inRange(0x8000, address, 0xFFFF)) {
        rom.PrgRom().at(address % rom.PrgRom().size()) = data;
    }

    // This should not happen. Check for bug in MMU and Cartridge
    spdlog::error("Invalid write address for Mapper 000: {:#6X}", address);
}
