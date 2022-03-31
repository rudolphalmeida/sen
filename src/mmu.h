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
        // TODO: Complete this
        //        Write(0x4015, 0x00);  // All Channels Disabled
        //        Write(0x4017, 0x00);  // Frame IRQ enabled
        //
        //        for (word i = 0x4000; i < 0x4014; i++) {
        //            Write(i, 0x00);
        //        }
    }

    [[nodiscard]] byte Read(word address) override;
    void Write(word address, byte data) override;

    // For Dummy Writes
    void SetLines(word address, word data);

    void Tick();

   private:
    std::shared_ptr<Cartridge> cart;
    // Internal RAM
    std::vector<byte> iram;

    word address_{};
    byte data_{};
};

#endif  // SEN_SRC_MMU_H_
