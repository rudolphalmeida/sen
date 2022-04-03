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

    cycles_t cyc{};

    union {
        struct {
            byte Carry : 1;
            byte Zero : 1;
            byte InterruptDisable : 1;
            byte Decimal : 1;
            byte B : 2;
            byte Overflow : 1;
            byte Negative : 1;
        } flags;
        byte value;
    } p{};

    void ExecuteOpcode(Opcode opcode);
    byte Fetch();

    // Addressing Modes
    word Indirect(word pointer);
    word ZeroPageIndexed(byte operand, byte offset);
    word Absolute();
    word AbsoluteIndexed(byte offset);
    word IndirectX();
    word IndirectY();

    // Opcodes
    void BCC(Opcode opcode);
    void BCS(Opcode opcode);
    void BNE(Opcode opcode);
    void CLC(Opcode opcode);
    void BEQ(Opcode opcode);
    void JMP(Opcode opcode);
    void JSR(Opcode opcode);
    void LDA(Opcode opcode);
    void LDX(Opcode opcode);
    void NOP(Opcode opcode);
    void SEC(Opcode opcode);
    void SED(Opcode opcode);  // This is only for completeness. Not used in NES
    void SEI(Opcode opcode);
    void STA(Opcode opcode);
    void STX(Opcode opcode);

    // Opcode Helpers
    void PerformRelativeBranch(bool condition);
};

#endif  // SEN_SRC_CPU_H_
