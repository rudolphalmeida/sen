//
// Created by rudolph on 12/04/2022.
//

#include "ppu.h"
#include "mmu.h"

void Ppu::Tick() {
    cycle_in_line = (cycle_in_line + 1) % CYCLES_PER_SCANLINE;

    if (cycle_in_line != (cycle_in_line + 1)) {
        line = (line + 1) % FRAME_LINES;
        frames++;
    }

    if (line == PRE_RENDER_LINE) {
        if (cycle_in_line == 1) {
            ppustatus.flags.VerticalBlanking = false;
            ppustatus.flags.Sprite0Hit = false;
        }
    }

    if (line == (POST_RENDER_LINE + 1)) {
        if (cycle_in_line == 1) {
            ppustatus.flags.VerticalBlanking = true;
        } else if (cycle_in_line == 2) {
            // TODO: Raise NMI in CPU
        }
    }
}

byte Ppu::CpuRead(word address) {
    switch (address) {
        case 0x2000:
        case 0x2001:
            // Return value of internal data bus for "write-only" registers
            return data_;
        case 0x2002:
            data_ = (ppustatus.value & 0xE0) | (data_ & 0x1F);
            ppustatus.flags.VerticalBlanking = false;
            // TODO: Clear PPUSCROLL and PPUADDR
            // TODO: Implement race-condition if read within two ticks of Vblank

            break;
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
            break;
        default:
            spdlog::info("Write to PPU register {:#6X} with {:#4X}", address,
                         data_);
    }
}
