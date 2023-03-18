#include <memory>

#include <spdlog/spdlog.h>

#include "mapper.hxx"

std::unique_ptr<Mapper> MapperFromInesNumber(byte mapper_number,
                                             size_t prg_rom_banks,
                                             size_t chr_rom_banks) {
    switch (mapper_number) {
        case 0x00:
            spdlog::info("Loading NROM mapper for cartridge");
            return std::unique_ptr<Mapper>(new Nrom(prg_rom_banks, chr_rom_banks));
        default:
            spdlog::error("Cartridge requires not implemented mapper {}", mapper_number);
            std::exit(-1);
    }
}

word Nrom::MapCpuAddr(word addr) {
    if (inRange<word>(0x8000, addr, 0xBFFF)) {
        return addr;
    } else if (inRange<word>(0xC000, addr, 0xFFFF)) {
        // If 16KB then the first 16KB bank is mirrored in this region
        return prg_rom_banks == 1 ? addr % 0x8000 : addr;
    } else {
        spdlog::error("Unknown CPU address {:#6X} to NROM", addr);
        return addr;
    }
}
