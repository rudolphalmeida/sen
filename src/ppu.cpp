//
// Created by rudolph on 12/04/2022.
//

#include "ppu.h"
#include "mmu.h"

void Ppu::Tick() {}

byte Ppu::CpuRead(word address) {
    switch (address) {
        case 0x2000:
            return ppuctrl.value;
        default:
            spdlog::info("Read from PPU register {:#6X}", address);
            return 0xFF;
    }
}

void Ppu::CpuWrite(word address, byte data) {
    switch (address) {
        case 0x2000:
            ppuctrl.value = data;
            break;
        default:
            spdlog::info("Write to PPU register {:#6X} with {:#4X}", address,
                         data);
    }
}
