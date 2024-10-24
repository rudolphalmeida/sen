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
            }

            if (InRange<unsigned int>(257, line_cycles, 320) && scanline != PRE_RENDER_SCANLINE) {
                const size_t sprite_index = (line_cycles - 257) / 8;
                const auto& sprite = sprite_index < secondary_oam.size() ?  secondary_oam[sprite_index] : Sprite { .y = 0xFF, .tile_index = 0xFF, .attribs = 0xFF, .x = 0xFF};

                scanline_sprites_tile_data.resize(secondary_oam.size());
                switch (const size_t cycle_into_sprite = (line_cycles - 257) % 8) {
                    case 1:
                    case 3:
                        // TODO: Garbage nametable read here
                        if (sprite_index < secondary_oam.size()) {
                            scanline_sprites_tile_data[sprite_index] = ActiveSprite {.tile_lsb = 0x00, .tile_msb = 0x00};
                        }
                        break;
                    case 5: {
                        // TODO: Sprite vertical flip
                        const auto data_ = PpuRead(PatternTableAddressSprite(sprite));
                        if (sprite_index < secondary_oam.size()) {
                            scanline_sprites_tile_data[sprite_index].tile_lsb = data_;
                        }
                        break;
                    }
                    case 7: {
                        const auto data_ = PpuRead(PatternTableAddressSprite(sprite) + 8);
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

size_t Ppu::PatternTableAddressSprite(const Sprite& sprite) const {
    return SpriteHeight() == 16 ? ((sprite.tile_index & 1) << 12) | ((sprite.tile_index & 0xFE) << 4) | ((scanline & 8) << 1) | (scanline & 7) : ((ppuctrl & 8) << 9) | (sprite.tile_index << 4) | (scanline & 7);
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
    const byte bg_pixel_msb = (bg_pattern_msb_shift_reg & (1 << (15 - fine_x))) ? 1 : 0;
    const byte bg_pixel_lsb = (bg_pattern_lsb_shift_reg & (1 << (15 - fine_x))) ? 1 : 0;
    const byte bg_pixel = (bg_pixel_msb << 1) | (bg_pixel_lsb);

    const byte bg_attrib_msb = (bg_attrib_msb_shift_reg & (1 << (7 - fine_x))) ? 1 : 0;
    const byte bg_attrib_lsb = (bg_attrib_lsb_shift_reg & (1 << (7 - fine_x))) ? 1 : 0;
    const byte bg_palette_offset = (bg_attrib_msb << 1) | bg_attrib_lsb;

    const byte bg_palette_address = (bg_palette_offset << 2) | bg_pixel;
    const byte bg_pixel_color_id = palette_table[bg_palette_address];

    const byte screen_x = line_cycles - 1;
    const byte screen_y = scanline;

    if (const auto sprite_index = SpriteForPixel(screen_x)) {
        const auto sprite = secondary_oam[*sprite_index];
        auto sprite_data = scanline_sprites_tile_data[*sprite_index];

        // Index from left
        const auto sprite_pixel_index = sprite.FlipHorizontal() ? (screen_x - sprite.x) : 7 - (screen_x - sprite.x);
        const byte sp_pixel_msb = ((sprite_data.tile_msb & (1 << sprite_pixel_index)) != 0) ? 1 : 0;
        const byte sp_pixel_lsb = ((sprite_data.tile_lsb & (1 << sprite_pixel_index)) != 0) ? 1 : 0;
        const byte sp_pixel = (sp_pixel_msb << 1) | (sp_pixel_lsb);
        const byte sp_palette_offset = sprite.PaletteIndex() + 4;
        const byte sp_palette_address = (sp_palette_offset << 2) | sp_pixel;
        const byte sp_pixel_color_id = palette_table[sp_palette_address];

        if (((ppustatus & 0x40) == 0x00) && sprite.tile_index == 0 && sp_pixel_color_id != 0 && bg_pixel_color_id != 0) {
            spdlog::debug("Sprite 0 hit at ({}, {})", screen_x, screen_y);
            ppustatus |= (0b1 << 6);
        }

        if (!bg_pixel_color_id) {
            if (!sp_pixel_color_id) {
                framebuffer[screen_y * NES_WIDTH + screen_x] = PpuRead(0x3F00);
            } else {
                framebuffer[screen_y * NES_WIDTH + screen_x] = sp_pixel_color_id;
            }
        } else {
            if (!sp_pixel_color_id) {
                framebuffer[screen_y * NES_WIDTH + screen_x] = bg_pixel_color_id;
            } else {
                framebuffer[screen_y * NES_WIDTH + screen_x] = sp_pixel_color_id;
            }
        }
    } else {
        framebuffer[screen_y * NES_WIDTH + screen_x] = bg_pixel_color_id;
    }

}

std::optional<size_t> Ppu::SpriteForPixel(const byte screen_x) const {
    std::optional<size_t> sprite_for_pixel = std::nullopt;

    for (int i = secondary_oam.size() - 1; i >= 0; i--) {
        auto& sprite = secondary_oam[i];
        if (screen_x >= sprite.x && ((screen_x - sprite.x) < 8)) {
            sprite_for_pixel = std::make_optional(static_cast<size_t>(i));
        }
    }

    return sprite_for_pixel;
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
    secondary_oam.clear();
}

void Ppu::EvaluateNextLineSprites() {
    for (const auto& sprite : oam) {
        if (const auto offset_y = scanline - sprite.y - 1; InRange<byte>(0, offset_y, 7)) {
            if (secondary_oam.size() == 8) {
                spdlog::debug("TODO: Implement sprite overflow");
                break;
            }
            secondary_oam.push_back(sprite);
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

    // Reset Vblank and Sprite 0 flag before rendering starts for the next frame
    if (scanline == PRE_RENDER_SCANLINE && line_cycles == VBLANK_SET_RESET_CYCLE) {
        ppustatus &= 0x1F;
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
            io_data_bus = reinterpret_cast<byte*>(oam.data())[oamaddr];
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
            if (((ppumask & 0x08) == 0x00) && ((data & 0x08) == 0x00)) {
                spdlog::debug("Rendering turned on at ({}, {})", line_cycles - 1, scanline);
            }
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
