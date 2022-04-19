//
// Created by rudolph on 12/04/2022.
//

#include "ppu.h"
#include "mmu.h"

void Ppu::Tick() {}

byte Ppu::CpuRead(word address) {
    switch (address) {
        case 0x2000:
        case 0x2001:
            // Return value of internal data bus for "write-only" registers
            return data_;
        default:
            spdlog::info("Read from PPU register {:#6X}", address);
            data_ = 0xFF;
    }

    return data_;
}

void Ppu::CpuWrite(word address, byte data) {
    data_ = data;
    switch (address) {
        case 0x2000:
            ppuctrl.value = data_;
            break;
        case 0x2001:
            ppumask.value = data_;
        default:
            spdlog::info("Write to PPU register {:#6X} with {:#4X}", address,
                         data_);
    }
}
