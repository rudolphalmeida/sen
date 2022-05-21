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
    Nes(const Options& options, Cpu&& cpu, std::shared_ptr<Mmu> mmu)
        : options{options}, mmu{std::move(mmu)}, cpu{std::move(cpu)} {}

    void Start();
    void RunFrame();

   private:
    [[maybe_unused]] const Options& options;

    std::shared_ptr<Mmu> mmu;
    Cpu cpu;
};

#endif  // SEN_SRC_NES_HXX_
