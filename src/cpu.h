//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_CPU_H_
#define SEN_SRC_CPU_H_

#include <utility>

#include "mmu.h"
#include "utils.h"

class Cpu {
   public:
    explicit Cpu(std::shared_ptr<Mmu> mmu);

    cycles_t Tick();

   private:
    std::shared_ptr<Mmu> mmu;

    byte a{}, x{}, y{}, s{};
    word pc{};

    union {
        struct {
            byte Negative : 1;
            byte Overflow : 1;
            byte B : 2;
            byte Decimal : 1;
            byte InterruptDisable : 1;
            byte Zero : 1;
            byte Carry : 1;
        } flags;
        byte value;
    } p{};

    cycles_t ExecuteOpcode();
    byte Fetch();

    // Addressing Modes

    // Opcodes
};

#endif  // SEN_SRC_CPU_H_
