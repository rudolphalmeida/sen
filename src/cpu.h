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

    void Start();
    void Reset();
    void Tick();

   private:
    std::shared_ptr<Mmu> mmu;

    byte a{}, x{}, y{}, s{};
    word pc{};

    union {
        struct {
            byte Carry : 1;             // Bit 0
            byte Zero : 1;              // Bit 1
            byte InterruptDisable : 1;  // Bit 2
            byte Decimal : 1;           // Bit 3
            byte B : 2;                 // Bit 4 & 5
            byte Overflow : 1;          // Bit 6
            byte Negative : 1;          // Bit 7
        } flags;
        byte value;
    } p{};

    void ExecuteOpcode(Opcode opcode);
    byte Fetch();

    // Addressing Modes
    word Indirect();
    word ZeroPageIndexed(byte offset);
    word Absolute();
    // Returns a effective address, page crossed pair
    std::pair<word, bool> AbsoluteIndexed(byte offset);
    word IndirectX();
    word IndirectY();

    // Opcodes
    void ADC(Opcode opcode);
    void AND(Opcode opcode);
    void ASL(Opcode opcode);
    void BCC(Opcode opcode);
    void BCS(Opcode opcode);
    void BEQ(Opcode opcode);
    void BIT(Opcode opcode);
    void BMI(Opcode opcode);
    void BNE(Opcode opcode);
    void BPL(Opcode opcode);
    void BVC(Opcode opcode);
    void BVS(Opcode opcode);
    void CLC(Opcode opcode);
    void CLD(Opcode opcode);  // This is only for completeness. Not used in NES
    void CLV(Opcode opcode);
    void CMP(Opcode opcode);
    void CPX(Opcode opcode);
    void CPY(Opcode opcode);
    void DEC(Opcode opcode);
    void DEX(Opcode opcode);
    void DEY(Opcode opcode);
    void EOR(Opcode opcode);
    void INC(Opcode opcode);
    void INX(Opcode opcode);
    void INY(Opcode opcode);
    void JMP(Opcode opcode);
    void JSR(Opcode opcode);
    void LDA(Opcode opcode);
    void LDX(Opcode opcode);
    void LDY(Opcode opcode);
    void LSR(Opcode opcode);
    void NOP(Opcode opcode);
    void ORA(Opcode opcode);
    void PHA(Opcode opcode);
    void PHP(Opcode opcode);
    void PLA(Opcode opcode);
    void PLP(Opcode opcode);
    void ROL(Opcode opcode);
    void ROR(Opcode opcode);
    void RTI(Opcode opcode);
    void RTS(Opcode opcode);
    void SBC(Opcode opcode);
    void SEC(Opcode opcode);
    void SED(Opcode opcode);  // This is only for completeness. Not used in NES
    void SEI(Opcode opcode);
    void STA(Opcode opcode);
    void STX(Opcode opcode);
    void STY(Opcode opcode);
    void TAX(Opcode opcode);
    void TAY(Opcode opcode);
    void TSX(Opcode opcode);
    void TXA(Opcode opcode);
    void TXS(Opcode opcode);
    void TYA(Opcode opcode);

    // Opcode Helpers
    byte FetchOperandForReadOpcodes(const Opcode& opcode, const char* repr);
    void PerformRelativeBranch(bool condition);
    void DoInc(byte& reg);
    void DoDec(byte& reg);
    void DoTransfer(byte& dest, byte src);
    void PerformShiftOrRotate(const Opcode& opcode, const char* repr);
};

#endif  // SEN_SRC_CPU_H_
