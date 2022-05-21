//
// Created by rudolph on 12/04/2022.
//

#ifndef SEN_SRC_PPU_H_
#define SEN_SRC_PPU_H_

#include <memory>
#include <utility>
#include <array>

#include "memory.h"

class Mmu;

class Ppu : public CpuBus, public PpuBus {
   public:
    Ppu(std::shared_ptr<Mmu> mmu) : mmu{std::move(mmu)} {}

    void Tick();

    // PPU Read/Write on the CPU address bus
    byte CpuRead(word address) override;
    void CpuWrite(word address, byte data) override;

    byte PpuRead(word address) override;
    void PpuWrite(word address, byte data) override;

    /* PPU Scanline Constants
     * Numerically the frame looks like:
     * FRAME_LINES (0..239)
     * POST_RENDER_LINE (240)
     * VBLANK_LINES (241..261)
     * PRE_RENDER_LINE (262)
     * */
    const uint FRAME_LINES = 262;
    const uint VISIBLE_FRAME_LINES = 240;
    const uint CYCLES_PER_SCANLINE = 341;
    const uint PRE_RENDER_LINE = 261;
    const uint POST_RENDER_LINE = 240;

   private:
    std::shared_ptr<Mmu> mmu;

    // PPUs internal data bus
    byte data_{};

    // Start out at the pre-render line (the -1 line, but I am using unsigned)
    uint frames{0};
    uint line{PRE_RENDER_LINE};
    uint cycle_in_line{0};

    bool InRenderingLines() const { return line < VISIBLE_FRAME_LINES; }

    bool InVerticalBlankLines() const {
        return line > POST_RENDER_LINE && line < PRE_RENDER_LINE;
    }

    // OAM
    struct OamEntry {
        byte y{};
        byte index{};
        union {
            struct {
                byte palette : 2;             // Palette (4 to 7) of sprite
                byte unimplemented : 3 = {};  // Unimplemented (Read 0)
                byte priority : 1;            // Priority (0: Front, 1: Back)
                bool horizontal_flip : 1;
                bool vertical_flip : 1;
            } flags;
            byte value;
        } attributes{};
        byte x{};
    };

    std::array<OamEntry, 64> oam{};
    std::array<byte, 0x1000> vram{};

    // PPU Registers
    union {
        struct {
            byte NametableBaseAddress : 2;       // 0x2000 + <value> * 0x400;
            byte IncrementVram : 1;              // 0: Add 1, 1: Add 32
            byte SpritePatternTableAddress : 1;  // For 8x8 sprites only
            byte BackgroundPatternTableAddress : 1;  // <value> * 0x1000
            byte SpriteSize : 1;                     // 0: 8x8, 1: 8x16
            byte PpuMasterSlaveSelect : 1;           // 0: read, 1: output
            byte VerticalBlankingInterrupt : 1;      // 0: Off, 1: On
        } flags;
        byte value;
    } ppuctrl{};

    union {
        struct {
            byte Greyscale : 1;     // Show image in grey-scale
            byte BgInLeftmost : 1;  // Show background in left-most 8 pixels
            byte SpritesInLeftmost : 1;  // Show sprites in left-most 8 pixels
            byte ShowBackground : 1;     // Show background layer
            byte ShowSprites : 1;        // Show sprite layer
            byte EmphasizeRed : 1;
            byte EmphasizeGreen : 1;
            byte EmphasizeBlue : 1;
        } flags;
        byte value;
    } ppumask{};

    union {
        struct {
            byte Unused_ : 5;  // LSB written to a PPU register most recently
            byte SpriteOverflow : 1;
            byte Sprite0Hit : 1;
            byte VerticalBlanking : 1;
        } flags;
        byte value;
    } ppustatus{.value = 0xA0};

    byte oamaddr{};
    byte oamdata{};

    byte scroll_x{};
    byte scroll_y{};
    bool write_to_x{true};

    word ppu_address{};
};

#endif  // SEN_SRC_PPU_H_
