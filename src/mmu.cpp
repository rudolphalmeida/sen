//
// Created by rudolph on 20/3/22.
//

#include "mmu.h"

void Mmu::SetLines(word address, word data) {
    data_ = data;
    address_ = address;

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest
}

byte Mmu::Read(word address) {
    address_ = address;

    if (inRange(0x0000, address_, 0x1FFF)) {
        data_ = iram.at(address_ & 0x7F);
    } else if (inRange(0x2000, address_, 0x3FFF)) {
        spdlog::info("Read from PPU register: {:#6X}", address_);
        data_ = 0x00;
    } else if (inRange(0x4000, address_, 0x4017)) {
        spdlog::info("Read from IO register: {:#6X}", address_);
        data_ = 0x00;
    } else if (inRange(0x4018, address_, 0x401F)) {
        spdlog::info("Read from Disabled functionality: {:#6X}", address_);
        data_ = 0x00;
    } else if (inRange(0x4020, address_, 0xFFFF)) {
        data_ = cart->Read(address_);
    } else {
        data_ = 0xFF;
    }

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest

    return data_;  // This should not be needed
}

void Mmu::Write(word address, byte data) {
    address_ = address;
    data_ = data;

    if (inRange(0x0000, address_, 0x1FFF)) {
        iram.at(address_ & 0x7FF) = data_;
    } else if (inRange(0x2000, address_, 0x3FFF)) {
        spdlog::info("Write to PPU register {:#6X} with {:#4X}", address_,
                     data_);
    } else if (inRange(0x4000, address_, 0x4017)) {
        spdlog::info("Write to IO register: {:#6X} with {:#4X}", address_,
                     data_);
    } else if (inRange(0x4018, address_, 0x401F)) {
        spdlog::info("Write to Disabled functionality {:#6X} with {:#4X}",
                     address_, data_);
    } else if (inRange(0x4020, address_, 0xFFFF)) {
        cart->Write(address_, data_);
    }

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest
}

void Mmu::Tick() {}
