#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include "mapper.hxx"
#include "cartridge.hxx"
#include "constants.hxx"

std::shared_ptr<Cartridge>
init_mapper(RomHeader header, std::vector<byte>&& prg_rom, std::vector<byte>&& chr_rom) {
    switch (header.mapper_number) { // TODO: Handle differences in iNES and NES2.0 headers
        case 0x00:
            spdlog::info("Loading NROM mapper for cartridge");
            return std::make_shared<Nrom>(header, std::move(prg_rom), std::move(chr_rom));
        case 0x01:
            spdlog::info("Loading MMC1 mapper for cartridge");
            return std::make_shared<Mmc1>(header, std::move(prg_rom), std::move(chr_rom));
        default:
            spdlog::error("Cartridge requires not implemented mapper {}", header.mapper_number);
            std::exit(-1);
    }
}
