//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MMU_H_
#define SEN_SRC_MMU_H_

#include "cartridge.h"
#include "memory.h"
#include "ppu.h"
#include "utils.h"

class Mmu : public CpuAddressSpace {
   public:
    explicit Mmu(std::shared_ptr<Cartridge> cart);

    // On the CPU address space
    byte Read(word address) override;
    void Write(word address, byte data) override;

    // For Dummy Writes
    [[maybe_unused]] void SetCpuLines(word address, byte data);
    byte RawCpuRead(word address);
    void RawCpuWrite(word address, byte data);

    [[nodiscard]] const std::vector<byte>& ChrRom() const {
        return cart->ChrRom();
    }

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
