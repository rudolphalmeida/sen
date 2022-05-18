//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MMU_H_
#define SEN_SRC_MMU_H_

#include "cartridge.h"
#include "memory.h"
#include "ppu.h"
#include "utils.h"

class Mmu : public CpuBus {
   public:
    explicit Mmu(std::shared_ptr<Cartridge> cart);

    // On the CPU address space
    byte CpuRead(word address) override;
    void CpuWrite(word address, byte data) override;

    // For Dummy Writes
    [[maybe_unused]] void SetCpuLines(word address, byte data);
    byte RawCpuRead(word address);
    void RawCpuWrite(word address, byte data);

    void Tick();

    bool NmiRequested() { return nmi_requested; }
    void RequestNmi();
    void NmiAcked();

    void IncCpuCycles(cycles_t by) { cpu_cycles += by; }
    void IncCpuCycles() { IncCpuCycles(1); }

    [[nodiscard]] cycles_t CpuCycles() const { return cpu_cycles; }
    [[nodiscard]] cycles_t PpuCycles() const { return ppu_cycles; }

   private:
    std::shared_ptr<Cartridge> cart;
    // Internal RAM
    std::vector<byte> iram;

    // Bus-attached components
    Ppu ppu;

    word address_{};
    byte data_{};

    cycles_t cpu_cycles{};
    cycles_t ppu_cycles{};

    bool nmi_requested{false};
};

#endif  // SEN_SRC_MMU_H_
