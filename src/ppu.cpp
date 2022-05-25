//
// Created by rudolph on 12/04/2022.
//

#include "ppu.h"
#include "mmu.h"

void Ppu::Tick() {
    cycle_in_line = (cycle_in_line + 1) % CYCLES_PER_SCANLINE;

    if (cycle_in_line == 0) {
        line = (line + 1) % FRAME_LINES;

        if (line == PRE_RENDER_LINE) {
            frames++;
            spdlog::debug("Frame Counter: {}", frames);
        }
    }

    if (line == PRE_RENDER_LINE) {
        if (cycle_in_line == 1) {
            ppu_status.flags.VerticalBlanking = false;
            ppu_status.flags.Sprite0Hit = false;
        }

        // Reference:OAMADDR is set to 0 during each of ticks 257-320 (the
        // sprite tile loading interval) of the pre-render and visible
        // scanlines.
        if (cycle_in_line >= 257 && cycle_in_line <= 320) {
            oam_addr = 0x00;
        }
    }

    if (InRenderingLines()) {
        if (cycle_in_line >= 257 && cycle_in_line <= 320) {
            oam_addr = 0x00;
        }
    }

    if (line == (POST_RENDER_LINE + 1)) {
        if (cycle_in_line == 1) {
            spdlog::debug("Beginning Vertical Blanking");
            ppu_status.flags.VerticalBlanking = true;
        } else if (cycle_in_line == 2) {
            mmu->RequestNmi();
        }
    }
}

byte Ppu::CpuRead(word address) {
    switch (address) {
        case 0x2000:
        case 0x2001:
        case 0x2003:
        case 0x2005:
        case 0x2006:
            // Return value of internal data bus for "write-only" registers
            return data_;
        case 0x2002:
            data_ = (ppu_status.value & 0xE0) | (data_ & 0x1F);

            // Clear registers and flags
            ppu_status.flags.VerticalBlanking = false;
            ppu_address = 0x0000;
            scroll_x = 0x00;
            scroll_y = 0x00;
            // TODO: Implement race-condition if read within two ticks of Vblank

            break;
        case 0x2004:
            if (InVerticalBlankLines()) {
                data_ = reinterpret_cast<unsigned char*>(oam.data())[oam_addr];
            } else {
                data_ = oam_data;
                oam_addr++;
            }
            break;
        case 0x2007: {
            data_ = PpuRead(ppu_address);
            if (ppu_ctrl.flags.IncrementVram) {
                ppu_address += 32;
            } else {
                ppu_address += 1;
            }
            break;
        }
        default:
            spdlog::debug("Read from PPU register {:#6X}", address);
            data_ = 0xFF;
    }

    return data_;
}

void Ppu::CpuWrite(word address, byte data) {
    data_ = data;
    switch (address) {
        case 0x2000:
            ppu_ctrl.value = data_;
            break;
        case 0x2001:
            ppu_mask.value = data_;
            break;
        case 0x2003:
            oam_addr = data_;
            break;
        case 0x2004:
            oam_data = data_;
            break;
        case 0x2005:
            if (write_to_x) {
                scroll_x = data_;
                write_to_x = false;
            } else {
                scroll_y = data_;
                write_to_x = true;
            }
            break;
        case 0x2006:
            ppu_address = (ppu_address << 8) | data_;
            break;
        case 0x2007: {
            PpuWrite(ppu_address, data);
            if (ppu_ctrl.flags.IncrementVram) {
                ppu_address += 32;
            } else {
                ppu_address += 1;
            }
            break;
        }
        default:
            spdlog::debug("Write to PPU register {:#6X} with {:#4X}", address,
                          data_);
    }
}

byte Ppu::PpuRead(word address) {
    address %= 0x4000;

    // TODO: PPUDATA read buffer for 0x0000 to 0x3EFF
    if (inRange(0x0000, address, 0x1FFF)) {
        return mmu->PpuRead(address);
    } else if (inRange(0x2000, address, 0x3EFF)) {
        return vram.at(address % 0x1000);
    } else if (inRange(0x3F00, address, 0x3FFF)) {
        spdlog::info("Read from Palette address: {:#6X}",
                     0x3F00 + (address % 0x20));
    }

    // Should not really happen
    return 0xFF;
}

void Ppu::PpuWrite(word address, byte data) {
    address %= 0x4000;

    if (inRange(0x0000, address, 0x1FFF)) {
        mmu->PpuWrite(address, data);
    } else if (inRange(0x2000, address, 0x3EFF)) {
        vram.at(address % 0x1000) = data;
    } else if (inRange(0x3F00, address, 0x3FFF)) {
        spdlog::info("Write to Palette address: {:#6X} with data: {:#4X}",
                     0x3F00 + (address % 0x20), data);
    }
}
