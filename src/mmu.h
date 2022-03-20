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
        : cart{std::move(cart)}, iram(0x800) {}

    [[nodiscard]] byte read(word address) const override;
    void write(word address, byte data) override;

   private:
    std::shared_ptr<Cartridge> cart;
    // Internal RAM
    std::vector<byte> iram;
};

#endif  // SEN_SRC_MMU_H_
