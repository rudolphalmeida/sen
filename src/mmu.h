//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MMU_H_
#define SEN_SRC_MMU_H_

#include "cartridge.h"
#include "memory.h"
#include "utils.h"

class Mmu : public Memory {
   public:
    explicit Mmu(std::shared_ptr<Cartridge> cart)
        : cart{std::move(cart)}, iram(0x800) {
        RawWrite(0x4015, 0x00);  // All Channels Disabled
        RawWrite(0x4017, 0x00);  // Frame IRQ enabled

        for (word i = 0x4000; i < 0x4014; i++) {
            RawWrite(i, 0x00);
        }
    }

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

    word address_{};
    byte data_{};
};

#endif  // SEN_SRC_MMU_H_
