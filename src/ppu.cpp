//
// Created by rudolph on 12/04/2022.
//

#include "ppu.h"

void Ppu::Tick() {}

byte Ppu::Read(word address) {
    spdlog::info("Read from PPU register: {:#6X}", address);
    return 0x00;
}
void Ppu::Write(word address, byte data) {
    spdlog::info("Write to PPU register {:#6X} with {:#4X}", address, data);
}
