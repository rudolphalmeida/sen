#include <memory>

#include <spdlog/spdlog.h>

#include "util.hxx"
#include "mapper.hxx"

std::shared_ptr<Cartridge> init_mapper(RomHeader header, const std::vector<byte>& prg_rom, const std::vector<byte>& chr_rom) {
    switch (header.mapper_number) {  // TODO: Handle differences in iNES and NES2.0 headers
        case 0x00:
            spdlog::info("Loading NROM mapper for cartridge");
            return std::make_shared<Nrom>(header, prg_rom, chr_rom);
        default:
            spdlog::error("Cartridge requires not implemented mapper {}", header.mapper_number);
            std::exit(-1);
    }
}
