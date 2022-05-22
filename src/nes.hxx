//
// Created by rudolph on 8/5/22.
//

#ifndef SEN_SRC_NES_HXX_
#define SEN_SRC_NES_HXX_

#include <memory>

#include "cpu.h"
#include "mmu.h"
#include "options.hxx"
#include "utils.h"

// Driver class for the NES
class Nes {
   public:
    Nes(const Options&, Cpu&& cpu, std::shared_ptr<Mmu> mmu)
        : mmu{std::move(mmu)}, cpu{std::move(cpu)} {}

    void Start();
    void RunFrame();

   private:
    std::shared_ptr<Mmu> mmu;
    Cpu cpu;
};

#endif  // SEN_SRC_NES_HXX_
