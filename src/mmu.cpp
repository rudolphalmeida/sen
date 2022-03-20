//
// Created by rudolph on 20/3/22.
//

#include "mmu.h"

byte Mmu::read(word address) const {
    if (inRange(0x0000, address, 0x1FFF)) {
        return iram.at(address % 0x7FF);
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
        return cart->read(address);
    }

    return 0xFF;  // This should not be needed
}

void Mmu::write(word address, byte data) {
    if (inRange(0x0000, address, 0x1FFF)) {
        iram.at(address % 0x7FF) = data;
    } else if (inRange(0x2000, address, 0x3FFF)) {
        spdlog::info("Write to PPU register {:#6X} with {:#4X}", address, data);
    } else if (inRange(0x4000, address, 0x4017)) {
        spdlog::info("Write to IO register: {:#6X} with {:#4X}", address, data);
    } else if (inRange(0x4018, address, 0x401F)) {
        spdlog::info("Write to Disabled functionality {:#6X} with {:#4X}",
                     address, data);
    } else if (inRange(0x4020, address, 0xFFFF)) {
        cart->write(address, data);
    }
}
