#include <spdlog/spdlog.h>

#include "ppu.hxx"
#include "util.hxx"

void Ppu::Tick() {
    TickCounters();

    if (ShowBackground() || ShowSprites()) {
        if (InRange<unsigned int>(0, scanline, POST_RENDER_SCANLINE - 1) ||
            scanline == PRE_RENDER_SCANLINE) {
            // PPU is accessing memory
            if (InRange<unsigned int>(1, line_cycles, 256)) {
                ShiftShifters();
                ReadNextTileData(line_cycles % 8);
                if (scanline != PRE_RENDER_SCANLINE) {
                    RenderPixel();
                }
            }

            if (line_cycles == 1 && scanline != PRE_RENDER_SCANLINE) {
                SecondaryOamClear();
            }

            if (line_cycles == 256) {
                FineYIncrement();
                if (scanline != PRE_RENDER_SCANLINE) {
                    LoadNextScanlineSprites();
                }
            }

            if (line_cycles == 257) {
                // hori(v) = hori(t)
                v.as_scroll.coarse_x_scroll = t.as_scroll.coarse_x_scroll;
            }

            if (InRange<unsigned int>(321, line_cycles, 336)) {
                // Fetch first two tiles on next scanline
                ReadNextTileData(line_cycles % 8);
            }

            if (line_cycles == 338 || line_cycles == 340) {
                // Unused nametable fetches
                // PpuRead(0x2000 | (v.value & 0x0FFF));
            }

            if (scanline == PRE_RENDER_SCANLINE && InRange<unsigned int>(280, line_cycles, 304)) {
                // vert(v) == vert(t) each tick
                v.as_scroll.coarse_y_scroll(t.as_scroll.coarse_y_scroll());
                v.as_scroll.fine_y_scroll = t.as_scroll.fine_y_scroll;
                v.as_scroll.nametable_select = t.as_scroll.nametable_select;
            }
        }
    } else {
        if (InRange<unsigned int>(0, scanline, POST_RENDER_SCANLINE - 1) &&
            InRange<unsigned int>(1, line_cycles, 256)) {
            byte pixel = PpuRead(0x3F00);

            byte screen_y = scanline;
            byte screen_x = line_cycles - 1;
            framebuffer[screen_y * NES_WIDTH + screen_x] = pixel;
        }
    }
}
void Ppu::FineYIncrement() {  // Fine Y increment
    if ((v.value & 0x7000) != 0x7000) {
        v.value += 0x1000;
    } else {
        v.value &= ~0x7000;
        byte y = v.as_scroll.coarse_y_scroll();
        if (y == 29) {
            y = 0;
            v.value ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y += 1;
        }
        v.as_scroll.coarse_y_scroll(y);
    }
}

void Ppu::ShiftShifters() {
    bg_pattern_msb_shift_reg <<= 1;
    bg_pattern_lsb_shift_reg <<= 1;

    bg_attrib_lsb_shift_reg <<= 1;
    bg_attrib_lsb_shift_reg |= ((bg_attrib_latch & (1 << 0)) != 0);

    bg_attrib_msb_shift_reg <<= 1;
    bg_attrib_msb_shift_reg |= ((bg_attrib_latch & (1 << 1)) != 0);
}

void Ppu::RenderPixel() {  // Output pixels
    byte pixel_msb = (bg_pattern_msb_shift_reg & (1 << (15 - fine_x))) ? 1 : 0;
    byte pixel_lsb = (bg_pattern_lsb_shift_reg & (1 << (15 - fine_x))) ? 1 : 0;
    byte background_pixel = (pixel_msb << 1) | (pixel_lsb);

    byte attrib_msb = (bg_attrib_msb_shift_reg & (1 << (7 - fine_x))) ? 1 : 0;
    byte attrib_lsb = (bg_attrib_lsb_shift_reg & (1 << (7 - fine_x))) ? 1 : 0;
    byte palette_offset = (attrib_msb << 1) | attrib_lsb;

    byte palette_address = (palette_offset << 2) | background_pixel;
    byte pixel = palette_table[palette_address];

    byte screen_y = scanline;
    byte screen_x = line_cycles - 1;
    framebuffer[screen_y * NES_WIDTH + screen_x] = pixel;
}

void Ppu::ReadNextTileData(unsigned int cycle) {
    switch (cycle) {  // 0, 1, ..., 7
        case 0:
            // Coarse X increment
            CoarseXIncrement();
            break;
        case 1:
            // Fetch NT byte
            tile_id_latch = PpuRead(0x2000 | (v.value & 0x0FFF));
            break;
        case 3: {
            // Fetch AT byte
            bg_attrib_data = PpuRead(0x23C0 | (v.value & 0x0C00) | ((v.value >> 4) & 0x38) |
                                     ((v.value >> 2) & 0x07));
            byte coarse_x = v.as_scroll.coarse_x_scroll;
            byte coarse_y = v.as_scroll.coarse_y_scroll();
            byte left_or_right = (coarse_x / 2) % 2;
            byte top_or_bottom = (coarse_y / 2) % 2;
            byte offset = ((top_or_bottom << 1) | left_or_right) * 2;
            bg_attrib_data = ((bg_attrib_data & (0b11 << offset)) >> offset) & 0b11;
            break;
        }
        case 5:
            // Fetch BG lsbits
            bg_pattern_lsb_latch =
                PpuRead(BgPatternTableAddress() + (static_cast<word>(tile_id_latch) << 4) +
                        v.as_scroll.fine_y_scroll);
            break;
        case 7:
            // Fetch BG msbits
            bg_pattern_msb_latch =
                PpuRead(BgPatternTableAddress() + (static_cast<word>(tile_id_latch) << 4) +
                        v.as_scroll.fine_y_scroll + 8);

            ReloadShiftersFromLatches();
            break;

        default:
            break;
    }
}
void Ppu::CoarseXIncrement() {
    if ((v.value & 0x001F) == 31) {
        v.value &= ~0x001F;
        v.value ^= 0x0400;
    } else {
        v.value += 1;
    }
}

void Ppu::SecondaryOamClear() {
    // Do not care about timing for this
    const auto secondary_oam_ptr = reinterpret_cast<byte *>(secondary_oam.data());
    for (int i = 0; i < sizeof(Sprite) * 4; i++) {
        secondary_oam_ptr[i] = 0xFF;
    }
}

void Ppu::LoadNextScanlineSprites() {
    // Do not care about timing for this
    int sprites_on_line = 0;
    const byte y = scanline + 1;
    for (const auto& sprite: oam) {
        if (sprite.y <= y && y < (sprite.y + 8)) {
            if (sprites_on_line == 8) {
                spdlog::info("More than 8 sprites on line");
                // TODO: Implement sprite overflow
                continue;
            }
            secondary_oam[sprites_on_line++] = sprite;
        }
    }
}

void Ppu::ReloadShiftersFromLatches() {
    bg_pattern_msb_shift_reg |= bg_pattern_msb_latch;
    bg_pattern_lsb_shift_reg |= bg_pattern_lsb_latch;

    bg_attrib_latch = bg_attrib_data;
}

void Ppu::TickCounters() {
    line_cycles++;

    if (scanline == PRE_RENDER_SCANLINE                        // The pre-render scanlines
        && line_cycles == (PPU_CLOCK_CYCLES_PER_SCANLINE - 1)  // cycle at the end
        && (frame_count % 2) != 0)                             // is skipped on odd frames
    {
        line_cycles++;
    }

    if (line_cycles == PPU_CLOCK_CYCLES_PER_SCANLINE) {
        scanline++;
        line_cycles = 0;

        if (scanline == SCANLINES_PER_FRAME) {
            frame_count++;
            scanline = 0;
        }
    }

    if (scanline == POST_RENDER_SCANLINE) {  // The PPU idles during the post-render line
        return;
    }

    if (scanline == VBLANK_START_SCANLINE && line_cycles == VBLANK_SET_RESET_CYCLE) {
        ppustatus |= 0x80;
        // Trigger NMI if enabled
        if (NmiAtVBlank()) {
            *nmi_requested = true;  // Trigger NMI in CPU
        }
    }

    // Reset Vblank flag before rendering starts for the next frame
    if (scanline == PRE_RENDER_SCANLINE && line_cycles == VBLANK_SET_RESET_CYCLE) {
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
            // TODO: Should return 0xFF if secondary OAM is being cleared
            io_data_bus = reinterpret_cast<byte *>(oam.data())[oamaddr];
            break;
        case 0x2007:
            if (const auto ppu_address = v.value; ppu_address > 0x3EFF) {  // Reading palettes
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
        default:
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
            reinterpret_cast<byte *>(oam.data())[oamaddr++] = data;
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

byte Ppu::PpuRead(word address) const {
    address &= 0x3FFF;
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        return cartridge->PpuRead(address);
    } else if (InRange<word>(0x2000, address, 0x2FFF)) {
        return vram[VramIndex(address)];
    } else if (InRange<word>(0x3000, address, 0x3EFF)) {
        return vram[address - 0x3000];
    } else if (InRange<word>(0x3F00, address, 0x3FFF)) {
        return palette_table[address & 0x1F];
    }
    return 0;
}

void Ppu::PpuWrite(word address, byte data) {
    address &= 0x3FFF;
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        cartridge->PpuWrite(address, data);
    } else if (InRange<word>(0x2000, address, 0x2FFF)) {
        vram[VramIndex(address)] = data;
    } else if (InRange<word>(0x3000, address, 0x3EFF)) {
        vram[address - 0x3000] = data;
    } else if (InRange<word>(0x3F00, address, 0x3FFF)) {
        palette_table[address & 0x1F] = data;
        if ((address & 0b11) == 0) {
            palette_table[(address & 0x1F) ^ 0x10] = data;
        }
    }
}

size_t Ppu::VramIndex(word address) const {
    switch (cartridge->NametableMirroring()) {
        case Horizontal:
            return (address & ~0x400) - 0x2000;
        case Vertical:
            return (address & ~0x800) - 0x2000;
        case FourScreenVram:
            break;
    }
    spdlog::error("Unknown mirroring option");
    return address - 0x2000;
}
