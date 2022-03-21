//
// Created by rudolph on 20/3/22.
//

#include "mmu.h"

byte Mmu::Read(word address) const {
    if (inRange(0x0000, address, 0x1FFF)) {
        return iram.at(address % 0x800);
    } else if (inRange(0x2000, address, 0x3FFF)) {
        spdlog::info("Read from PPU register: {:#6X}", address);
        return 0xFF;
    } else if (inRange(0x4000, address, 0x4017)) {
        spdlog::info("Read from IO register: {:#6X}", address);
        return 0xFF;
    } else if (inRange(0x4018, address, 0x401F)) {
        spdlog::info("Read from Disabled functionality: {:#6X}", address);
        return 0xFF;
    } else if (inRange(0x4020, address, 0xFFFF)) {
        return cart->Read(address);
    }

    return 0xFF;  // This should not be needed
}

void Mmu::Write(word address, byte data) {
    if (inRange(0x0000, address, 0x1FFF)) {
        iram.at(address % 0x800) = data;
    } else if (inRange(0x2000, address, 0x3FFF)) {
        spdlog::info("Write to PPU register {:#6X} with {:#4X}", address, data);
    } else if (inRange(0x4000, address, 0x4017)) {
        spdlog::info("Write to IO register: {:#6X} with {:#4X}", address, data);
    } else if (inRange(0x4018, address, 0x401F)) {
        spdlog::info("Write to Disabled functionality {:#6X} with {:#4X}",
                     address, data);
    } else if (inRange(0x4020, address, 0xFFFF)) {
        cart->Write(address, data);
    }
}
