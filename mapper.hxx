#pragma once

#include <cstddef>
#include <memory>

#include "constants.hxx"

class Mapper {
   private:
    size_t prg_rom_banks{};
    size_t chr_rom_banks{};

   public:
    Mapper(size_t prg_rom_banks, size_t chr_rom_banks)
        : prg_rom_banks{prg_rom_banks}, chr_rom_banks{chr_rom_banks} {}

    virtual word MapCpuAddr(word addr) = 0;
    virtual word MapPpuAddr(word addr) = 0;

    virtual ~Mapper() = default;
};

std::unique_ptr<Mapper> MapperFromInesNumber(byte mapper_number,
                                             size_t prg_rom_banks,
                                             size_t chr_rom_banks);
