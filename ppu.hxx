#pragma once

#include <spdlog/spdlog.h>

#include "constants.hxx"

class Ppu {
   public:
    void Tick() {}

    byte CpuRead(word address) {
        spdlog::debug("Read from not implented PPU address {:#06X}", address);
        return 0xFF;
    }

    void CpuWrite(word address, byte data) {
        spdlog::debug("Write to not implented PPU address {:#06X} with {:#04X}", address, data);
    }
};