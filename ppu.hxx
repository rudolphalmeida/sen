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
    std::array<byte, 32> palette_table{};
    std::array<byte, 2048> vram{};
    std::array<byte, 256> oam{};

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

            byte coarse_y_scroll() const { return coarse_y_scroll_high << 3 | coarse_y_scroll_low; }
        } as_scroll;
    } v{.value = 0x0000}, t{.value = 0x0000};  // Current, Temporary VRAM address (15 bits)
    byte fine_x{};                             // (x) Fine X (3 bits)
    bool write_toggle{false};                  // (w) 1 bit

    uint64_t frame_count{};  // Alse used to determine if even or odd frame

    // The first `Tick()` will put us at the start of pre-render line (261)
    unsigned int scanline{260};
    unsigned int cycles_into_scanline{340};

    std::shared_ptr<Cartridge> cartridge{};
    std::shared_ptr<bool> nmi_requested{};

    // Properties for the PPU
    word BaseNametableAddress() const {
        switch (ppuctrl & 0b11) {
            case 0:
                return 0x2000;
            case 1:
                return 0x2400;
            case 2:
                return 0x2800;
            case 3:
                return 0x2C00;
        }

        return 0x2000;  // Shouldn't be possible
    }

    byte VramAddressIncrement() const { return (ppuctrl & 0x04) != 0x00 ? 32 : 1; }
    word SpritePatternTableAddress() const { return (ppuctrl & 0x08) != 0x00 ? 0x1000 : 0x0000; }
    word BgPatternTableAddress() const { return (ppuctrl & 0x10) != 0x00 ? 0x1000 : 0x0000; }
    byte SpriteHeight() const { return (ppuctrl & 0x20) != 0x00 ? 16 : 8; }
    bool NmiAtVBlank() const { return (ppuctrl & 0x80) != 0x00; }

    bool Grayscale() const { return (ppumask & 0x01) != 0x00; }
    bool ShowBackgroundInLeft() const { return (ppumask & 0x02) != 0x00; }
    bool ShowSpritesInLeft() const { return (ppumask & 0x04) != 0x00; }
    bool ShowBackground() const { return (ppumask & 0x08) != 0x00; }
    bool ShowSprites() const { return (ppumask & 0x10) != 0x00; }
    bool EmphasizeRed() const { return (ppumask & 0x20) != 0x00; }
    bool EmphasizeGreen() const { return (ppumask & 0x40) != 0x00; }
    bool EmphasizeBlue() const { return (ppumask & 0x80) != 0x00; }

    // TODO: Unstub me
    bool InVblank() const { return false; }

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

    byte PpuRead(word address);
    void PpuWrite(word address, byte data);
};