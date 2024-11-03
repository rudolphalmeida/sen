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

            if (line_cycles == 64 && scanline != PRE_RENDER_SCANLINE) {
                // TODO: This is where the actual secondary OAM clear happens
                // SecondaryOamClear();
            }

            if (line_cycles == 256) {
                // TODO: Do the secondary OAM clear here since we use the data structure when rendering
                if (scanline != PRE_RENDER_SCANLINE) {
                    SecondaryOamClear();
                    EvaluateNextLineSprites();
                }
                FineYIncrement();
            }

            if (line_cycles == 257) {
                // hori(v) = hori(t)
                v.as_scroll.coarse_x_scroll = t.as_scroll.coarse_x_scroll;
                v.as_scroll.nametable_select = (v.as_scroll.nametable_select & 0b10) | (t.as_scroll.nametable_select & 0b01);
            }

            if (InRange<unsigned int>(257, line_cycles, 320) && scanline != PRE_RENDER_SCANLINE) {
                const size_t sprite_index = (line_cycles - 257) / 8;
                const auto& sprite = sprite_index < secondary_oam.size() ?  secondary_oam[sprite_index].second : Sprite { .y = 0xFF, .tile_index = 0xFF, .attribs = 0xFF, .x = 0xFF};

                unsigned int offset_into_sprite;
                if (sprite.FlipVertical()) {
                    offset_into_sprite = SpriteHeight() - 1 - (scanline - sprite.y);
                } else {
                    offset_into_sprite = scanline - sprite.y;
                }

                word line_pattern_table_addr;
                if (SpriteHeight() == 16) {
                    line_pattern_table_addr = (((sprite.tile_index & 0x01) ? 0x1000 : 0x0000) | ((sprite.tile_index & ~0x01) << 4)) + (offset_into_sprite >= 8 ? offset_into_sprite + 8 : offset_into_sprite);
                } else {
                    line_pattern_table_addr = ((sprite.tile_index << 4) | SpritePatternTableAddress()) + offset_into_sprite;
                }

                switch (const size_t cycle_into_sprite = (line_cycles - 257) % 8) {
                    case 1:
                    case 3:
                        // TODO: Garbage nametable read here
                        if (sprite_index < secondary_oam.size()) {
                            scanline_sprites_tile_data[sprite_index] = ActiveSprite {.tile_lsb = 0x00, .tile_msb = 0x00};
                        }
                        break;
                    case 5: {
                        const auto data_ = PpuRead(line_pattern_table_addr);
                        if (sprite_index < secondary_oam.size()) {
                            scanline_sprites_tile_data[sprite_index].tile_lsb = data_;
                        }
                        break;
                    }
                    case 7: {
                        const auto data_ = PpuRead(line_pattern_table_addr + 8);
                        if (sprite_index < secondary_oam.size()) {
                            scanline_sprites_tile_data[sprite_index].tile_msb = data_;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            if (InRange<unsigned int>(321, line_cycles, 336)) {
                // Fetch first two tiles on next scanline
                ReadNextTileData(line_cycles % 8);
            }

            if (line_cycles == 338 || line_cycles == 340) {
                // Unused nametable fetches
                auto _ = PpuRead(0x2000 | (v.value & 0x0FFF));
            }

            if (scanline == PRE_RENDER_SCANLINE && InRange<unsigned int>(280, line_cycles, 304)) {
                // vert(v) == vert(t) each tick
                v.as_scroll.coarse_y_scroll(t.as_scroll.coarse_y_scroll());
                v.as_scroll.fine_y_scroll = t.as_scroll.fine_y_scroll;
                v.as_scroll.nametable_select = (v.as_scroll.nametable_select & 0b01) | (t.as_scroll.nametable_select & 0b10);
            }
        }
    } else {
        if (InRange<unsigned int>(0, scanline, POST_RENDER_SCANLINE - 1) &&
            InRange<unsigned int>(1, line_cycles, 256)) {
            const byte pixel = PpuRead(0x3F00);

            const byte screen_y = scanline;
            const byte screen_x = line_cycles - 1;
            framebuffer[screen_y * NES_WIDTH + screen_x] = pixel;
        }
    }
}

void Ppu::FineYIncrement() {  // Fine Y increment
    if ((v.value & 0x7000) != 0x7000) {
        v.value = (v.value + 0x1000) & 0x7FFF;
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
    const byte bg_pixel_msb = (bg_pattern_msb_shift_reg & (1 << (15 - fine_x))) ? 1 : 0;
    const byte bg_pixel_lsb = (bg_pattern_lsb_shift_reg & (1 << (15 - fine_x))) ? 1 : 0;
    const byte bg_pixel = (bg_pixel_msb << 1) | (bg_pixel_lsb);

    const byte screen_x = line_cycles - 1;
    const byte screen_y = scanline;

    bool had_sprite_on_pixel = false;
    bool rendered_sprite_on_pixel = false;
    for (int i = static_cast<int>(secondary_oam.size()) - 1; i >= 0; i--) {
        const auto& [oam_index, sprite] = secondary_oam[i];

        if (screen_x < sprite.x || ((screen_x - sprite.x) >= 8)) {
            continue;
        }
        had_sprite_on_pixel = true;

        auto [tile_lsb, tile_msb] = scanline_sprites_tile_data[i];

        const auto sprite_pixel_index = sprite.FlipHorizontal() ? (screen_x - sprite.x) : 7 - (screen_x - sprite.x);
        const byte sp_pixel_msb = ((tile_msb & (1 << sprite_pixel_index)) != 0) ? 1 : 0;
        const byte sp_pixel_lsb = ((tile_lsb & (1 << sprite_pixel_index)) != 0) ? 1 : 0;
        const byte sp_pixel = (sp_pixel_msb << 1) | (sp_pixel_lsb);

        if (((ppustatus & 0x40) == 0x00) && oam_index == 0 && sp_pixel != 0 && bg_pixel != 0) {
            ppustatus |= 0x40;
        }

        if (!sp_pixel || (sprite.BgOverSprite() && bg_pixel)) {
            continue;
        }

        const byte sp_palette_offset = sprite.PaletteIndex() + 4;
        const byte sp_palette_address = (sp_palette_offset << 2) | sp_pixel;
        const byte sp_pixel_color_id = palette_table[sp_palette_address];

        framebuffer[screen_y * NES_WIDTH + screen_x] = sp_pixel_color_id;
        rendered_sprite_on_pixel = true;
    }

    if (!had_sprite_on_pixel && !bg_pixel) {
        framebuffer[screen_y * NES_WIDTH + screen_x] = PpuRead(0x3F00);
    } else if (!rendered_sprite_on_pixel) {
        const byte bg_attrib_msb = (bg_attrib_msb_shift_reg & (1 << (7 - fine_x))) ? 1 : 0;
        const byte bg_attrib_lsb = (bg_attrib_lsb_shift_reg & (1 << (7 - fine_x))) ? 1 : 0;
        const byte bg_palette_offset = (bg_attrib_msb << 1) | bg_attrib_lsb;
        const byte bg_palette_address = bg_pixel == 0 ? bg_pixel : (bg_palette_offset << 2) | bg_pixel;
        const byte bg_pixel_color_id = palette_table[bg_palette_address];

        framebuffer[screen_y * NES_WIDTH + screen_x] = bg_pixel_color_id;
    }
}

void Ppu::ReadNextTileData(const unsigned int cycle) {
    switch (cycle) {  // 0, 1, ..., 7
        case 2:
            // Fetch NT byte
            tile_id_latch = PpuRead(0x2000 | (v.value & 0x0FFF));
            break;
        case 4: {
            // Fetch AT byte
            bg_attrib_data = PpuRead(0x23C0 | (v.value & 0x0C00) | ((v.value >> 4) & 0x38) |
                                     ((v.value >> 2) & 0x07));
            const byte coarse_x = v.as_scroll.coarse_x_scroll;
            const byte coarse_y = v.as_scroll.coarse_y_scroll();
            const byte left_or_right = (coarse_x / 2) % 2;
            const byte top_or_bottom = (coarse_y / 2) % 2;
            const byte offset = ((top_or_bottom << 1) | left_or_right) * 2;
            bg_attrib_data = ((bg_attrib_data & (0b11 << offset)) >> offset) & 0b11;
            break;
        }
        case 6:
            // Fetch BG ls bits
            bg_pattern_lsb_latch =
                PpuRead(BgPatternTableAddress() + (static_cast<word>(tile_id_latch) << 4) +
                        v.as_scroll.fine_y_scroll);
            break;
        case 0:
            // Fetch BG ms bits
            bg_pattern_msb_latch =
                PpuRead(BgPatternTableAddress() + (static_cast<word>(tile_id_latch) << 4) +
                        v.as_scroll.fine_y_scroll + 8);

            ReloadShiftersFromLatches();
            CoarseXIncrement();
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
        v.value = (v.value + 1) & 0x7FFF;
    }
}

void Ppu::SecondaryOamClear() {
    secondary_oam.clear();
}

void Ppu::EvaluateNextLineSprites() {
    for (size_t i = 0; i < 64; i++) {
        if (const auto& sprite = oam[i];
            scanline >= sprite.y && ((scanline - sprite.y) < SpriteHeight())) {
            if (secondary_oam.size() == 8) {
                spdlog::debug("TODO: Implement sprite overflow");
                break;
            }
            secondary_oam.emplace_back(i, sprite);
        }
    }
    scanline_sprites_tile_data.resize(secondary_oam.size());
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

    // Reset Vblank and Sprite 0 flag before rendering starts for the next frame
    if (scanline == PRE_RENDER_SCANLINE && line_cycles == VBLANK_SET_RESET_CYCLE) {
        ppustatus &= 0x1F;
    }
}

byte Ppu::CpuRead(const word address) {
    switch (0x2000 + (address & 0b111)) {
        case 0x2002:
            io_data_bus = (ppustatus & 0xE0) | (io_data_bus & 0x1F);
            ppustatus &= 0x7F;  // Reading this register clears bit 7
            write_toggle = false;
            break;
        case 0x2004:
            // TODO: Should return 0xFF if secondary OAM is being cleared
            io_data_bus = reinterpret_cast<byte*>(oam.data())[oamaddr];
            break;
        case 0x2007:
            // https://www.nesdev.org/wiki/PPU_scrolling#$2007_reads_and_writes
            if ((ShowBackground() || ShowSprites()) && (InRange<unsigned>(0, scanline, POST_RENDER_SCANLINE - 1) || scanline == PRE_RENDER_SCANLINE)) {
                CoarseXIncrement();
                FineYIncrement();
                io_data_bus = 0x00;
            } else {
                if (const auto ppu_address = v.value & 0x3FFF; ppu_address >= 0x3F00) {  // Reading palettes
                    io_data_bus = PpuRead(ppu_address);
                } else {
                    // Use previously read value from buffer and reset buffer
                    if (ppudata_buf) {
                        io_data_bus = ppudata_buf.value();
                    }
                    ppudata_buf.emplace(PpuRead(ppu_address));
                }
                v.value = (v.value + VramAddressIncrement()) & 0x7FFF;
            }
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
            reinterpret_cast<byte*>(oam.data())[oamaddr++] = data;
            break;
        case 0x2005:
            if (write_toggle) {  // Second Write
                t.as_scroll.fine_y_scroll = data & 0b111;
                t.as_scroll.coarse_y_scroll((data & 0xF8) >> 3);
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
            v.value = (v.value + VramAddressIncrement()) & 0x7FFF;
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
        return PpuRead(address - 0x1000);
    } else if (InRange<word>(0x3F00, address, 0x3FFF)) {
        return palette_table[address & 0x1F];
    }
    return 0;
}

void Ppu::PpuWrite(word address, const byte data) {
    address &= 0x3FFF;
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        cartridge->PpuWrite(address, data);
    } else if (InRange<word>(0x2000, address, 0x2FFF)) {
        vram[VramIndex(address)] = data;
    } else if (InRange<word>(0x3000, address, 0x3EFF)) {
        PpuWrite(address - 0x1000, data);
    } else if (InRange<word>(0x3F00, address, 0x3FFF)) {
        palette_table[address & 0x1F] = data;
        if ((address & 0b11) == 0) {
            palette_table[(address & 0x1F) ^ 0x10] = data;
        }
    }
}

size_t Ppu::VramIndex(const word address) const {
    switch (cartridge->NametableMirroring()) {
        case Horizontal:
            return ((address >> 1) & 0x400) | (address & 0x3FF);
        case Vertical:
            return address & 0x7FF;
        case FourScreenVram:
            // TODO Mirroring
            break;
    }
    spdlog::error("Unknown mirroring option");
    return address - 0x2000;
}
