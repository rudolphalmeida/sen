//
// Created by rudolph on 20/3/22.
//

#include <array>

#include "cpu.h"

/// Base timings of each opcode without any page faults or branching
std::array<cycles_t, 256> OPCODE_TIMINGS{
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5,
    3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
    2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, 2, 6, 2, 6, 3, 3, 3, 3,
    2, 2, 2, 2, 4, 4, 4, 4, 2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6,
    2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

Cpu::Cpu(std::shared_ptr<Mmu> mmu) : mmu(std::move(mmu)) {
    // Set Power-Up State. Reference:
    // https://www.nesdev.org/wiki/CPU_power_up_state
    a = x = y = 0x00;
    p = 0x24;  // Differs from nesdev to match with nestest.nes
    s = 0xFD;

    // TODO: Determine this by reading the reset vector. For nestest
    // hardcoded to 0xC000
    pc = 0xC000;

    this->mmu->Write(0x4015, 0x00);  // All Channels Disabled
    this->mmu->Write(0x4017, 0x00);  // Frame IRQ enabled

    for (word i = 0x4000; i < 0x4014; i++) {
        this->mmu->Write(i, 0x00);
    }
}

cycles_t Cpu::Tick() {
    return 0;
}

cycles_t Cpu::ExecuteOpcode() {
    return 0;
}

byte Cpu::Fetch() {
    return mmu->Read(pc++);
}

// Addressing Modes

// Opcodes
