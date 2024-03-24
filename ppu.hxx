#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "constants.hxx"

class Ppu {
   private:
    struct Sprite {
        byte y;
        byte tile_index;
        byte attribs;
        byte x;

        byte PaletteIndex() const {
            return attribs & 0b11;
        }

        byte Priority() const {
            return (attribs & 0x20) >> 5;
        }

        bool FlipHorizontal() const {
            return (attribs & 0x40) >> 6;
        }

        bool FlipVertical() const {
            return (attribs & 0x80) >> 7;
        }
    };

    std::array<byte, 2048> vram{};
    std::array<Sprite, 64> oam{};
    std::array<Sprite, 8> secondary_oam{};

    // Due to mirroring of the palette indices we don't need exactly 32 bytes
    // Still keep it at 32 for easy indexing using the address
    std::array<byte, 32> palette_table{};

    // Open bus value for the CPU <-> PPU data bus
    byte io_data_bus{};

    byte ppuctrl{};
    byte ppumask{};
    byte ppustatus{0x1F};
    byte oamaddr{};
    std::optional<byte> ppudata_buf = std::nullopt;  // PPUDATA read buffer

    // Rendering registers
    union {
        word value;
        struct {
            byte low;
            byte high;
        } as_bytes;
        struct {
            byte coarse_x_scroll : 5;
            byte coarse_y_scroll_low : 3;
            byte coarse_y_scroll_high : 2;
            byte nametable_select : 2;
            byte fine_y_scroll : 3;
            byte : 1;

            [[nodiscard]] byte coarse_y_scroll() const {
                return (coarse_y_scroll_high << 3) | coarse_y_scroll_low;
            }
            void coarse_y_scroll(byte data) {
                coarse_y_scroll_low = data;
                coarse_y_scroll_high = (data >> 3);
            }
        } as_scroll;
    } v{.value = 0x0000}, t{.value = 0x0000};  // Current, Temporary VRAM address (15 bits)
    byte fine_x{};                             // (x) Fine X (3 bits)
    bool write_toggle{false};                  // (w) 1 bit

    // Latch for tile ID of the next tile to be drawn. Updated every 8-cycles
    // Tiles will be fetched from this address and set tp the lower 8-bits
    // of the shift registers
    byte tile_id_latch{};

    /* These contain the pattern table data for two tiles and their corresponding latches
     * Every 8 cycles, the data for the next tile will be loaded into the upper 8 bits of these
     *  The pixel to render will be fetched from the lower 8 bits
     */
    byte bg_pattern_msb_latch{}, bg_pattern_lsb_latch{};
    word bg_pattern_msb_shift_reg{}, bg_pattern_lsb_shift_reg{};

    // Latch for palette attribute for next tile. Updated every 8-cycles.
    // Only 2 bits
    byte bg_attrib_latch{};
    // Temp to hold palette attribute data until reloading
    byte bg_attrib_data{};

    /* These contain the palette attributes for the lower 8 pixels of the 16-bit shift registers.
     * These are fed from the latch which contains the palette attribute for the next tile
     */
    byte bg_attrib_msb_shift_reg{}, bg_attrib_lsb_shift_reg{};

    uint64_t frame_count{};  // Also used to determine if even or odd frame

    // The first `Tick()` will put us at the start of pre-render line (261)
    unsigned int scanline{260};
    unsigned int line_cycles{340};

    std::shared_ptr<Cartridge> cartridge{};
    std::shared_ptr<bool> nmi_requested{};

    std::array<byte, NES_WIDTH * NES_HEIGHT> framebuffer{};

    // Properties for the PPU
    [[nodiscard]] word BaseNametableAddress() const {
        switch (ppuctrl & 0b11) {
            case 0:
                return 0x2000;
            case 1:
                return 0x2400;
            case 2:
                return 0x2800;
            case 3:
                return 0x2C00;
            default:
                spdlog::error("Unreachable case");
                return 0x2000;
        }
    }

    [[nodiscard]] byte VramAddressIncrement() const { return (ppuctrl & 0x04) != 0x00 ? 32 : 1; }
    [[nodiscard]] word SpritePatternTableAddress() const {
        return (ppuctrl & 0x08) != 0x00 ? 0x1000 : 0x0000;
    }
    [[nodiscard]] word BgPatternTableAddress() const {
        return (ppuctrl & 0x10) != 0x00 ? 0x1000 : 0x0000;
    }
    [[nodiscard]] byte SpriteHeight() const { return (ppuctrl & 0x20) != 0x00 ? 16 : 8; }
    [[nodiscard]] bool NmiAtVBlank() const { return (ppuctrl & 0x80) != 0x00; }

    [[nodiscard]] bool Grayscale() const { return (ppumask & 0x01) != 0x00; }
    [[nodiscard]] bool ShowBackgroundInLeft() const { return (ppumask & 0x02) != 0x00; }
    [[nodiscard]] bool ShowSpritesInLeft() const { return (ppumask & 0x04) != 0x00; }
    [[nodiscard]] bool ShowBackground() const { return (ppumask & 0x08) != 0x00; }
    [[nodiscard]] bool ShowSprites() const { return (ppumask & 0x10) != 0x00; }
    [[nodiscard]] bool EmphasizeRed() const { return (ppumask & 0x20) != 0x00; }
    [[nodiscard]] bool EmphasizeGreen() const { return (ppumask & 0x40) != 0x00; }
    [[nodiscard]] bool EmphasizeBlue() const { return (ppumask & 0x80) != 0x00; }

    [[nodiscard]] bool InVblank() const {
        // Bit 7 is set during VBlank
        return (ppustatus & 0x80) != 0x00;
    }

    void TickCounters();
    void ShiftShifters();
    void ReloadShiftersFromLatches();
    void ReadNextTileData(unsigned int cycle);
    void RenderPixel();
    void FineYIncrement();
    void CoarseXIncrement();
    void SecondaryOamClear();
    void LoadNextScanlineSprites();

    size_t VramIndex(word address) const;

    friend class Debugger;

   public:
    const unsigned int SCANLINES_PER_FRAME = 262;
    const unsigned int PPU_CLOCK_CYCLES_PER_SCANLINE = 341;
    const unsigned int PRE_RENDER_SCANLINE = 261;
    const unsigned int POST_RENDER_SCANLINE = 240;
    const unsigned int VBLANK_START_SCANLINE = 241;
    const unsigned int VBLANK_SET_RESET_CYCLE = 1;

    Ppu() = default;

    Ppu(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<bool> nmi_requested)
        : cartridge{std::move(cartridge)}, nmi_requested{std::move(nmi_requested)} {}

    void Tick();

    byte CpuRead(word address);
    void CpuWrite(word address, byte data);

    byte PpuRead(word address) const;
    void PpuWrite(word address, byte data);
};