#pragma once

#include <spdlog/spdlog.h>
#include <cstdint>

#include "cartridge.hxx"
#include "constants.hxx"
#include "controller.hxx"
#include "ppu.hxx"

class Bus {
   private:
    std::shared_ptr<Cartridge> cartridge;
    std::vector<byte> internal_ram;
    std::shared_ptr<Ppu> ppu;
    std::shared_ptr<Controller> controller;

   public:
    uint64_t cycles{0};  // CPU cycles executed since startup

    Bus(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<Ppu> ppu, std::shared_ptr<Controller> controller)
        : cartridge{std::move(cartridge)}, internal_ram(0x800, 0xFF), ppu{std::move(ppu)}, controller{std::move(controller)} {
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
#ifndef CPU_TEST
        // Each CPU cycle is 3 PPU cycles
        ppu->Tick();
        ppu->Tick();
        ppu->Tick();
#endif
    }

    void PerformOamDma(byte high);

    friend class Debugger;
};
