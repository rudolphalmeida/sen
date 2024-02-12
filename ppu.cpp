#include <spdlog/spdlog.h>

#include "ppu.hxx"
#include "util.hxx"

void Ppu::Tick() {
    TickCounters();

    if (ShowBackground() || ShowSprites()) {
        if (InRange<unsigned int>(0, scanline, POST_RENDER_SCANLINE - 1) ||
            scanline == PRE_RENDER_SCANLINE) {
            // PPU is accessing memory
            if (InRange<unsigned int>(1, cycles_into_scanline, 256)) {
                ReadPixelData((cycles_into_scanline - 1) % 8);
                if (scanline != PRE_RENDER_SCANLINE) {
                    // Output pixels
                    byte pixel_msb = (bg_pattern_msb_shift_reg & (1 << fine_x)) ? 1 : 0;
                    byte pixel_lsb = (bg_pattern_lsb_shift_reg & (1 << fine_x)) ? 1 : 0;
                    byte pixel = (pixel_msb << 1) | (pixel_lsb);
                    byte screen_y = scanline;
                    byte screen_x = cycles_into_scanline - 1;
                    framebuffer[screen_y * NES_WIDTH + screen_x] = pixel;
                }

                fine_x = (fine_x + 1) & 0b111;  // 0-7
            }
            if (cycles_into_scanline == 256) {
                // Fine Y increment
                if ((v.value & 0x7000) != 0x7000) {
                    v.value += 0x1000;
                } else {
                    v.value &= ~0x7000;
                    int y = (v.value & 0x03E0) >> 5;
                    if (y == 29) {
                        y = 0;
                        v.value ^= 0x0800;
                    } else if (y == 31) {
                        y = 0;
                    } else {
                        y += 1;
                    }
                    v.value = (v.value & ~0x03E0) | (y << 5);
                }
            }

            if (cycles_into_scanline == 257) {
                // hori(v) = hori(t)
                v.as_scroll.coarse_x_scroll = t.as_scroll.coarse_x_scroll;
            }

            if (InRange<unsigned int>(321, cycles_into_scanline, 336)) {
                // Fetch first two tiles on next scanline
                ReadPixelData((cycles_into_scanline - 321) % 8);
            }

            if (cycles_into_scanline == 338 || cycles_into_scanline == 340) {
                // Unused nametable fetches
                PpuRead(0x2000 | (v.value & 0x0FFF));
            }

            if (scanline == PRE_RENDER_SCANLINE &&
                InRange<unsigned int>(280, cycles_into_scanline, 304)) {
                // vert(v) == vert(t) each tick
                v.as_scroll.coarse_y_scroll(t.as_scroll.coarse_y_scroll());
            }
        }
    } else {
        // TODO: Output pixels with color from 0x3F00 (universal background color)
    }
}

void Ppu::ReadPixelData(unsigned int cycle) {
    switch (cycle) {  // 0, 1, ..., 7
        case 1:
            // Fetch NT byte
            tile_id_latch = PpuRead(0x2000 | (v.value & 0x0FFF));
            break;
        case 3:
            // Fetch AT byte
            bg_attrib_latch = PpuRead(0x23C0 | (v.value & 0x0C00) | ((v.value >> 4) & 0x38) |
                                      ((v.value >> 2) & 0x07));
            break;
        case 5:
            // Fetch BG lsbits
            bg_pattern_lsb_latch = PpuRead(BgPatternTableAddress() + (tile_id_latch << 4) + v.as_scroll.fine_y_scroll);
            break;
        case 7:
            // Fetch BG msbits
            bg_pattern_msb_latch = PpuRead(BgPatternTableAddress() + (tile_id_latch << 4) + v.as_scroll.fine_y_scroll + 8);
            // Coarse X increment
            if ((v.value & 0x001F) == 31) {
                v.value &= ~0x001F;
                v.value ^= 0x0400;
            } else {
                v.value += 1;
            }

            ReloadShiftersFromLatches();
            break;

        default:
            break;
    }
}

void Ppu::ReloadShiftersFromLatches() {
    bg_pattern_lsb_shift_reg >>= 8;
    bg_pattern_msb_shift_reg >>= 8;

    bg_pattern_msb_shift_reg |= (static_cast<word>(bg_pattern_msb_latch) << 8);
    bg_pattern_lsb_shift_reg |= (static_cast<word>(bg_pattern_lsb_latch) << 8);

    // TODO: Load attribute shifters
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
            // Might need to be delayed by 3 cycles
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
            spdlog::debug("Write to not implemented PPU address {:#06X} with {:#04X}", address,
                          data);
    }
}

byte Ppu::PpuRead(word address) {
    address &= 0x3FFF;
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        return cartridge->PpuRead(address);
    } else if (InRange<word>(0x2000, address, 0x2FFF)) {
        return vram[address - 0x2000];
    } else if (InRange<word>(0x3000, address, 0x3EFF)) {
        return vram[address - 0x3000];
    } else if (InRange<word>(0x3F00, address, 0x3FFF)) {
        return palette_table[Ppu::PaletteIndex(address)];
    }
}

void Ppu::PpuWrite(word address, byte data) {
    address &= 0x3FFF;
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        cartridge->PpuWrite(address, data);
    } else if (InRange<word>(0x2000, address, 0x2FFF)) {
        vram[address - 0x2000] = data;
    } else if (InRange<word>(0x3000, address, 0x3EFF)) {
        vram[address - 0x3000] = data;
    } else if (InRange<word>(0x3F00, address, 0x3FFF)) {
        palette_table[Ppu::PaletteIndex(address)] = data;
    }
}

size_t Ppu::PaletteIndex(word address) {
    size_t index = address & 0x1F;  // Only lower 5 bits are used for palette indices

    if (index & ~0b11) {  // The lower two bits are 0 (00, 04, 08, 0C, 10, 14, 18, 1C)
        // Map these to 00, 04, 08, 0C
        return index & 0xF;
    }

    return address % 32;
}
