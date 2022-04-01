//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_CPU_H_
#define SEN_SRC_CPU_H_

#include <utility>

#include "mmu.h"
#include "opcode.h"
#include "utils.h"

class Cpu {
   public:
    explicit Cpu(std::shared_ptr<Mmu> mmu);

    void Tick();

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

    void ExecuteOpcode(Opcode opcode);
    byte Fetch();

    // Addressing Modes
    word Indirect(word pointer);
    word ZeroPageIndexed(byte operand, byte offset);
    word Absolute();
    std::pair<word, word> AbsoluteIndexed(byte offset);

    // Opcodes
    void JMP(Opcode opcode);
    void JSR(Opcode opcode);
    void LDX(Opcode opcode);
    void STX(Opcode opcode);
};

#endif  // SEN_SRC_CPU_H_
