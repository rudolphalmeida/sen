//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_CPU_H_
#define SEN_SRC_CPU_H_

#include "mmu.h"
#include "utils.h"

enum class FlagRegister : byte {
    Carry = (1 << 0),
    Zero = (1 << 1),
    InterruptDisable = (1 << 2),
    Overflow = (1 << 6),
    Negative = (1 << 7),
};

class Cpu {
   public:
    explicit Cpu(std::shared_ptr<Mmu> mmu);

    [[nodiscard]] bool FlagStatus(FlagRegister flag) const {
        return (p & (byte)flag) != 0;
    }

    uint Tick();

   private:
    std::shared_ptr<Mmu> mmu;

    byte a{}, x{}, y{}, s{}, p{};
    word pc{};

    void SetFlag(FlagRegister flag, bool value) {
        if (value) {
            p |= (byte)flag;
        } else {
            p &= ~((byte)flag);
        }
    }

    uint ExecuteOpcode();
    byte Fetch();

    // Addressing Modes
    word Absolute();
    word Indirect();

    // Opcodes
    uint JMP(byte opcode);
};

#endif  // SEN_SRC_CPU_H_
