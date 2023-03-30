#pragma once

#include <memory>

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "constants.hxx"

class Ppu {
   private:
    std::shared_ptr<Cartridge> cartridge{};
    std::shared_ptr<bool> nmi_requested{};

   public:
    Ppu() = default;

    Ppu(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<bool> nmi_requested)
        : cartridge{std::move(cartridge)}, nmi_requested{std::move(nmi_requested)} {}

    void Tick() {}

    byte CpuRead(word address) {
        spdlog::debug("Read from not implented PPU address {:#06X}", address);
        return 0xFF;
    }

    void CpuWrite(word address, byte data) {
        spdlog::debug("Write to not implented PPU address {:#06X} with {:#04X}", address, data);
    }
};