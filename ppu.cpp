#include <spdlog/spdlog.h>

#include "ppu.hxx"
#include "util.hxx"

void Ppu::Tick() {
    TickCounters();

    if (ShowBackground() || ShowSprites()) {
        if (inRange<unsigned int>(0, scanline, POST_RENDER_SCANLINE - 1) ||
            scanline == PRE_RENDER_SCANLINE) {
            // PPU is accessing memory

            if (inRange<unsigned int>(1, cycles_into_scanline, 256)) {
                // PPU is outputting pixels
            }

            if (inRange<unsigned int>(321, cycles_into_scanline, 336)) {
                // Fetch first two tiles on next scanline
            }

            if (inRange<unsigned int>(337, cycles_into_scanline, 340)) {
                // Unused nametable fetches
            }

            if (scanline == PRE_RENDER_SCANLINE &&
                inRange<unsigned int>(280, cycles_into_scanline, 304)) {
                // vert(v) == vert(t) each tick
            }
        }
    }
}

void Ppu::TickCounters() {
    cycles_into_scanline++;

    if (scanline == PRE_RENDER_SCANLINE                                 // The pre-render scanlines
        && cycles_into_scanline == (PPU_CLOCK_CYCLES_PER_SCANLINE - 1)  // cycle at the end
        && (frame_count % 2) != 0)                                      // is skipped on odd frames
    {
        cycles_into_scanline++;
    }

    if (cycles_into_scanline == PPU_CLOCK_CYCLES_PER_SCANLINE) {
        scanline++;
        cycles_into_scanline = 0;

        if (scanline == SCANLINES_PER_FRAME) {
            frame_count++;
            scanline = 0;
        }
    }

    if (scanline == POST_RENDER_SCANLINE) {  // The PPU idles during the post-render line
        return;
    }

    if (scanline == VBLANK_START_SCANLINE && cycles_into_scanline == VBLANK_SET_RESET_CYCLE) {
        ppustatus |= 0x80;
        // Trigger NMI if enabled
        if (NmiAtVBlank()) {
            *nmi_requested = true;  // Trigger NMI in CPU
        }
    }

    // Reset Vblank flag before rendering starts for the next frame
    if (scanline == PRE_RENDER_SCANLINE && cycles_into_scanline == VBLANK_SET_RESET_CYCLE) {
        ppustatus &= 0x7F;
    }
}

byte Ppu::CpuRead(word address) {
    switch (0x2000 + (address & 0b111)) {
        case 0x2002:
            io_data_bus = (ppustatus & 0xE0) | (io_data_bus & 0x1F);
            ppustatus &= 0x7F;  // Reading this register clears bit 7
            write_toggle = false;
            break;
        case 0x2004:
            io_data_bus = oam[oamaddr];
            break;
        case 0x2007:
            auto ppu_address = v.value;
            if (ppu_address > 0x3EFF) {  // Reading palettes
                io_data_bus = PpuRead(ppu_address);
            } else if (ppudata_buf) {
                // Use previously read value from buffer and reset buffer
                io_data_bus = ppudata_buf.value();
                ppudata_buf.reset();
            } else {
                // Read value into PPUDATA read buffer
                ppudata_buf.emplace(PpuRead(ppu_address));
            }
            v.value += VramAddressIncrement();
            break;
    }

    return io_data_bus;
}

void Ppu::CpuWrite(word address, byte data) {
    io_data_bus = data;
    switch (0x2000 + (address & 0b111)) {
        case 0x2000:
            if (InVblank()                       // If the PPU is in Vblank
                && ((ppustatus & 0x80) != 0x00)  // and the PPUSTATUS vblank flag is still set
                && ((ppuctrl & 0x80) != 0x00)    // and changing the NMI flag in PPUCTRL from 0
                && ((data & 0x80) == 0x80))      // to 1
            {
                *nmi_requested = true;  // will immediately generate an NMI
            }
            ppuctrl = data;
            t.as_scroll.nametable_select = data & 0b11;
            break;
        case 0x2001:
            ppumask = data;
            break;
        case 0x2003:
            oamaddr = data;
            break;
        case 0x2004:
            // TODO: Ignore these writes if during rendering
            oam[oamaddr++] = data;
            break;
        case 0x2005:
            if (write_toggle) {  // Second Write
                t.as_scroll.fine_y_scroll = data & 0b111;
                t.as_scroll.coarse_y_scroll_low = (data & 0x38) >> 3;
                t.as_scroll.coarse_y_scroll_high = (data & 0xC0) >> 6;
            } else {  // First Write
                fine_x = data & 0b111;
                t.as_scroll.coarse_x_scroll = (data & 0xF8) >> 3;
            }
            write_toggle = !write_toggle;
            break;
        case 0x2006:
            if (write_toggle) {  // Second Write
                t.as_bytes.low = data;
                v.value = t.value;
            } else {  // First Write
                t.as_bytes.high = data & 0x3F;
            }
            write_toggle = !write_toggle;
            break;
        case 0x2007:
            PpuWrite(v.value, data);
            v.value += VramAddressIncrement();
            break;
        default:
            spdlog::debug("Write to not implented PPU address {:#06X} with {:#04X}", address, data);
    }
}

byte Ppu::PpuRead(word address) {
    if (inRange<word>(0x0000, address, 0x1FFF)) {
        return cartridge->PpuRead(address);
    } else if (inRange<word>(0x2000, address, 0x2FFF)) {
        return vram[address - 0x2000];
    } else if (inRange<word>(0x3000, address, 0x3EFF)) {
        return vram[address - 0x3000];
    } else if (inRange<word>(0x3F00, address, 0x3FFF)) {
        return palette_table[address % 32];
    } else {
        return PpuRead(address & 0x3FFF);  // Addresses should be mapped to 0x0000-0x3FFF already
    }
}

void Ppu::PpuWrite(word address, byte data) {
    if (inRange<word>(0x0000, address, 0x1FFF)) {
        cartridge->PpuWrite(address, data);
    } else if (inRange<word>(0x2000, address, 0x2FFF)) {
        vram[address - 0x2000] = data;
    } else if (inRange<word>(0x3000, address, 0x3EFF)) {
        vram[address - 0x3000] = data;
    } else if (inRange<word>(0x3F00, address, 0x3FFF)) {
        palette_table[address % 32] = data;
    } else {
        PpuWrite(address & 0x3FFF, data);  // Addresses should be mapped to 0x0000-0x3FFF already
    }
}
