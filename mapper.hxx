#pragma once

#include <spdlog/spdlog.h>
#include <cstddef>
#include <memory>

#include "constants.hxx"
#include "util.hxx"

class Mapper {
   public:
    const size_t prg_rom_banks;
    const size_t chr_rom_banks;

    Mapper(size_t prg_rom_banks, size_t chr_rom_banks)
        : prg_rom_banks{prg_rom_banks}, chr_rom_banks{chr_rom_banks} {}

    virtual word MapCpuAddr(word addr) = 0;
    virtual word MapPpuAddr(word addr) { return addr; };

    virtual ~Mapper() = default;
};

std::unique_ptr<Mapper> MapperFromInesNumber(byte mapper_number,
                                             size_t prg_rom_banks,
                                             size_t chr_rom_banks);

// Mapper 0
// Most basic with no switchable PRG ROM with 16KB and 32KB sizes
// and no CHR ROM banking and 8KB fixed size
class Nrom : public Mapper {
   public:
    Nrom(size_t prg_rom_banks, size_t chr_rom_banks) : Mapper{prg_rom_banks, chr_rom_banks} {}

    word MapCpuAddr(word addr);
};
