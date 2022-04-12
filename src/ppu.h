//
// Created by rudolph on 12/04/2022.
//

#ifndef SEN_SRC_PPU_H_
#define SEN_SRC_PPU_H_

#include <memory>
#include <utility>
#include "memory.h"

class Mmu;

class Ppu : public Memory {
   public:
    explicit Ppu(std::shared_ptr<Mmu> mmu) : mmu{std::move(mmu)} {}

    void Tick();
    byte Read(word address) override;
    void Write(word address, byte data) override;

   private:
    std::shared_ptr<Mmu> mmu;
};

#endif  // SEN_SRC_PPU_H_
