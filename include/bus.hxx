#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "apu.hxx"
#include "cartridge.hxx"
#include "constants.hxx"
#include "controller.hxx"
#include "ppu.hxx"

class Bus {
  private:
    std::shared_ptr<Cartridge> cartridge;
    std::vector<byte> internal_ram;
    std::shared_ptr<Ppu> ppu;
    std::shared_ptr<Apu> apu;
    std::shared_ptr<Controller> controller;

  public:
    uint64_t cycles{0}; // CPU cycles executed since startup

    Bus(std::shared_ptr<Cartridge> cartridge,
        std::shared_ptr<Ppu> ppu,
        std::shared_ptr<Apu> apu,
        std::shared_ptr<Controller> controller) :
        cartridge{std::move(cartridge)},
        internal_ram(0x800, 0xFF),
        ppu{std::move(ppu)},
        apu{std::move(apu)},
        controller{std::move(controller)} {
        spdlog::debug("Initialized system bus");
    }

    [[nodiscard]] byte cpu_read(word address) const;
    void cpu_write(word address, byte data);

    byte ticked_cpu_read(const word address) {
        tick();
        return cpu_read(address);
    }

    void ticked_cpu_write(const word address, const byte data) {
        tick();
        cpu_write(address, data);
    }

    void tick() {
        cycles++;
#ifndef CPU_TEST
        // Each CPU cycle is 3 PPU cycles
        ppu->Tick();
        ppu->Tick();
        ppu->Tick();

        apu->Tick(cycles);
#endif
    }

    void perform_oam_dma(byte high);

    friend class Debugger;
};
