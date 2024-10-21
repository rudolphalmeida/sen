#include <memory>

#include <spdlog/spdlog.h>

#include "util.hxx"
#include "mapper.hxx"

std::unique_ptr<Mapper> MapperFromInesNumber(byte mapper_number,
                                             size_t prg_rom_banks,
                                             size_t chr_rom_banks) {
    switch (mapper_number) {
        case 0x00:
            spdlog::info("Loading NROM mapper for cartridge");
            return std::make_unique<Nrom>(prg_rom_banks, chr_rom_banks);
        default:
            spdlog::error("Cartridge requires not implemented mapper {}", mapper_number);
            std::exit(-1);
    }
}

unsigned int Nrom::MapCpuAddr(word address) {
    if (InRange<word>(0x8000, address, 0xFFFF)) {
        return address % (16384 * prg_rom_banks);
    } else {
        spdlog::error("Unknown CPU address {:#06X} to NROM", address);
        return 0x0000;
    }
}
