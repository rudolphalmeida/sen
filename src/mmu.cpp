//
// Created by rudolph on 20/3/22.
//

#include "mmu.h"

[[maybe_unused]] void Mmu::SetLines(word address, byte data) {
    data_ = data;
    address_ = address;

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest
}

byte Mmu::Read(word address) {
    address_ = address;
    data_ = RawRead(address);

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest

    return data_;  // This should not be needed
}

byte Mmu::RawRead(word address) {
    if (inRange(0x0000, address, 0x1FFF)) {
        return iram.at(address & 0x7FF);
    } else if (inRange(0x2000, address, 0x3FFF)) {
        spdlog::info("Read from PPU register: {:#6X}", address);
        return 0x00;
    } else if (inRange(0x4000, address, 0x4017)) {
        spdlog::info("Read from IO register: {:#6X}", address_);
        return 0x00;
    } else if (inRange(0x4018, address, 0x401F)) {
        spdlog::info("Read from Disabled functionality: {:#6X}", address);
        return 0x00;
    } else if (inRange(0x4020, address, 0xFFFF)) {
        return cart->Read(address);
    } else {
        return 0xFF;
    }
}

void Mmu::Write(word address, byte data) {
    address_ = address;
    data_ = data;

    RawWrite(address, data);

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest
}

void Mmu::RawWrite(word address, byte data) {
    if (inRange(0x0000, address, 0x1FFF)) {
        iram.at(address & 0x7FF) = data;
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

void Mmu::Tick() {}
