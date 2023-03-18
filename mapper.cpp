#include <spdlog/spdlog.h>

#include "mapper.hxx"

std::unique_ptr<Mapper> MapperFromInesNumber(byte mapper_number,
                                             size_t prg_rom_banks,
                                             size_t chr_rom_banks) {
    switch (mapper_number) {
        default:
            spdlog::error("Cartridge requires not implemented mapper {}", mapper_number);
            std::exit(-1);
    }
}