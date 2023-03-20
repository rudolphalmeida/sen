#pragma once

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "constants.hxx"

class Bus {
   private:
    Cartridge cartridge;

    std::vector<byte> internal_ram;

   public:
    unsigned int cycles{7};  // CPU cycles executed since startup

    Bus(Cartridge&& cartridge) : cartridge{std::move(cartridge)}, internal_ram(0x800, 0xFF) {
        spdlog::debug("Initialized system bus");
    }

    byte CpuRead(word addr);
    void CpuWrite(word addr, byte data);
};
