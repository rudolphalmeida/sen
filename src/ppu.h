//
// Created by rudolph on 12/04/2022.
//

#ifndef SEN_SRC_PPU_H_
#define SEN_SRC_PPU_H_

#include <memory>
#include <utility>
#include "memory.h"

class Mmu;

class Ppu : public CpuBus {
   public:
    Ppu(std::shared_ptr<Mmu> mmu) : mmu{std::move(mmu)} {}

    void Tick();

    // PPU Read/Write on the CPU address bus
    byte CpuRead(word address) override;
    void CpuWrite(word address, byte data) override;

   private:
    std::shared_ptr<Mmu> mmu;

    // PPU Registers
    union {
        struct flags {
            byte NametableBaseAddress : 2;       // 0x2000 + <value> * 0x400;
            byte IncrementVram : 1;              // 0: Add 1, 1: Add 32
            byte SpritePatternTableAddress : 1;  // For 8x8 sprites only
            byte BackgroundPatternTableAddress : 1;  // <value> * 0x1000
            byte SpriteSize : 1;                     // 0: 8x8, 1: 8x16
            byte PpuMasterSlaveSelect : 1;           // 0: read, 1: output
            byte VerticalBlankingInterrupt : 1;      // 0: Off, 1: On
        };
        byte value;
    } ppuctrl{};
};

#endif  // SEN_SRC_PPU_H_
