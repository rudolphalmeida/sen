#pragma once

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "constants.hxx"
#include "ppu.hxx"

class Bus {
   private:
    Cartridge cartridge;
    std::vector<byte> internal_ram;
    std::shared_ptr<Ppu> ppu;

   public:
    unsigned int cycles{7};  // CPU cycles executed since startup

    Bus(Cartridge&& cartridge, std::shared_ptr<Ppu> ppu)
        : cartridge{std::move(cartridge)}, internal_ram(0x800, 0xFF), ppu{std::move(ppu)} {
        spdlog::debug("Initialized system bus");
    }

    byte UntickedCpuRead(word address);
    void UntickedCpuWrite(word address, byte data);

    byte CpuRead(word address) {
        Tick();
        return UntickedCpuRead(address);
    }

    void CpuWrite(word address, byte data) {
        Tick();
        UntickedCpuWrite(address, data);
    }

    void Tick() {
        cycles++;
        ppu->Tick();
    }
};
