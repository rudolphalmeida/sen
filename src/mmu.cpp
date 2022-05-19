//
// Created by rudolph on 20/3/22.
//

#include "mmu.h"

Mmu::Mmu(std::shared_ptr<Cartridge> cart)
    : cart{std::move(cart)}, iram(0x800), ppu{std::shared_ptr<Mmu>(this)} {
    RawCpuWrite(0x4015, 0x00);  // All Channels Disabled
    RawCpuWrite(0x4017, 0x00);  // Frame IRQ enabled

    for (word i = 0x4000; i < 0x4014; i++) {
        RawCpuWrite(i, 0x00);
    }
}

[[maybe_unused]] void Mmu::SetCpuLines(word address, byte data) {
    data_ = data;
    address_ = address;

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest
}

byte Mmu::CpuRead(word address) {
    address_ = address;
    data_ = RawCpuRead(address);

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest

    return data_;
}

byte Mmu::RawCpuRead(word address) {
    if (inRange(0x0000, address, 0x1FFF)) {
        return iram.at(address & 0x7FF);
    } else if (inRange(0x2000, address, 0x3FFF)) {
        return ppu.CpuRead(address);
    } else if (inRange(0x4000, address, 0x4017)) {
        spdlog::debug("Read from IO register: {:#6X}", address_);
        return 0x00;
    } else if (inRange(0x4018, address, 0x401F)) {
        spdlog::debug("Read from Disabled functionality: {:#6X}", address);
        return 0x00;
    } else if (inRange(0x4020, address, 0xFFFF)) {
        return cart->CpuRead(address);
    } else {
        return 0xFF;
    }
}

void Mmu::CpuWrite(word address, byte data) {
    address_ = address;
    data_ = data;

    RawCpuWrite(address, data);

    Tick();  // Every CPU cycle R/W from memory. We use that to drive the rest
}

void Mmu::RawCpuWrite(word address, byte data) {
    if (inRange(0x0000, address, 0x1FFF)) {
        iram.at(address & 0x7FF) = data;
    } else if (inRange(0x2000, address, 0x3FFF)) {
        ppu.CpuWrite(address, data);
    } else if (inRange(0x4000, address, 0x4017)) {
        spdlog::debug("Write to IO register: {:#6X} with {:#4X}", address, data);
    } else if (inRange(0x4018, address, 0x401F)) {
        spdlog::debug("Write to Disabled functionality {:#6X} with {:#4X}",
                     address, data);
    } else if (inRange(0x4020, address, 0xFFFF)) {
        cart->CpuWrite(address, data);
    }
}

void Mmu::Tick() {
    IncCpuCycles();

    // Every CPU cycle is three PPU cycles
    for (int i = 0; i < 3; i++) {
        ppu.Tick();
    }
    ppu_cycles += 3;
}

void Mmu::RequestNmi() {
    spdlog::debug("NMI requested");
    nmi_requested = true;
}

void Mmu::NmiAcked() {
    spdlog::debug("NMI acknowledged");
    nmi_requested = false;
}
