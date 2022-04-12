//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MMU_H_
#define SEN_SRC_MMU_H_

#include "cartridge.h"
#include "memory.h"
#include "ppu.h"
#include "utils.h"

class Mmu : public Memory {
   public:
    explicit Mmu(std::shared_ptr<Cartridge> cart);

    byte Read(word address) override;
    void Write(word address, byte data) override;

    // For Dummy Writes
    [[maybe_unused]] void SetLines(word address, byte data);
    byte RawRead(word address);
    void RawWrite(word address, byte data);

    void Tick();

   private:
    std::shared_ptr<Cartridge> cart;
    // Internal RAM
    std::vector<byte> iram;

    // Bus-attached components
    Ppu ppu;

    word address_{};
    byte data_{};
};

#endif  // SEN_SRC_MMU_H_
