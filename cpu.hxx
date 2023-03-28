#pragma once

#include <cassert>
#include <memory>
#include <tuple>

#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "constants.hxx"

// Effective address and if there was a page crossing
using EffectiveAddress = std::tuple<word, bool>;

enum class OpcodeClass {
    // Add Memory to Accumulator With Carry
    ADC,
    // AND Memory with Accumulator
    AND,
    // Shift Left One Bit (Accumulator or Memory)
    ASL,
    // Branch on Carry Clear
    BCC,
    // Branch on Carry Set
    BCS,
    // Branch on Result Zero
    BEQ,
    // Test Bits in Memory with Accumulator
    BIT,
    // Branch on Resul Minus
    BMI,
    // Branch on Result Not Zero
    BNE,
    // Branch on Result Plus
    BPL,
    // Branch on Overflow Clear
    BVC,
    // Branch on Overflow Set
    BVS,
    // Clear Carry Flag
    CLC,
    // Clear Decimal Mode
    CLD,
    // Clear Overflow Bug
    CLV,
    // Compare Memory with Accumulator
    CMP,
    // Compare Memory with Index X
    CPX,
    // Compare Memory with Index Y
    CPY,
    // Decrement Memory by One
    DEC,
    // Decrement Index X by One
    DEX,
    // Decrement Index Y by One
    DEY,
    // Exclusive-OR Memory with Accumulator
    EOR,
    // Increment Memory by One
    INC,
    // Increment Index X by One
    INX,
    // Increment Index Y by One
    INY,
    // Not really an opcode, jams the CPU when executed
    JAM,
    // Jump
    JMP,
    // Jump, Saving Return Address
    JSR,
    // Load Accumulator with Memory
    LDA,
    // Load X from Memory
    LDX,
    // Load Y from Memory
    LDY,
    // Shift One Bit Right (Memory or Accumulator)
    LSR,
    // No Operation
    NOP,
    // OR Memory with Accumulator
    ORA,
    // Push Accumulator To Stack
    PHA,
    // Push Processor Status on Stack
    PHP,
    // Pull Accumulator From Stack
    PLA,
    // Pull Processor Status From Stack
    PLP,
    // Rotate One Bit Left (Memory or Accumulator)
    ROL,
    // Rotate One Bit Right (Memory or Accumulator)
    ROR,
    // Return from Interrupt
    RTI,
    // Return from Subroutine
    RTS,
    // Subtract Memory from Accumulator with Borrow
    SBC,
    // Set Carry Flag
    SEC,
    // Set Decimal Flag
    SED,
    // Set Interrupt Disable Status
    SEI,
    // Store Accumulator in Memory
    STA,
    // Store X to Memory
    STX,
    // Store Y to Memory
    STY,
    // Transfer Accumulator to Index X
    TAX,
    // Transfer Accumulator to Index Y
    TAY,
    // Transfer Stack Pointer to Index X
    TSX,
    // Transfer Index X to Accumulator
    TXA,
    // Transfer Index X to Stack Register
    TXS,
    // Transfer Index Y to Accumulator
    TYA,
};

enum class AddressingMode {
    Accumulator,
    Absolute,
    AbsoluteXIndexed,
    AbsoluteYIndexed,
    Immediate,
    Implied,
    Indirect,
    IndirectX,
    IndirectY,
    Relative,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
};

struct Opcode {
    OpcodeClass opcode_class;
    byte opcode;
    AddressingMode addressing_mode;

    size_t length;
    unsigned int cycles;
};

enum class StatusFlag : byte {
    Carry = (1 << 0),             // C
    Zero = (1 << 1),              // Z
    InterruptDisable = (1 << 2),  // I
    Decimal = (1 << 3),           // D
    _B = (0b11 << 4),             // No CPU effect, bits 45
    Overflow = (1 << 6),          // V
    Negative = (1 << 7),          // N
};

class Cpu {
   private:
    // Some of these values are hardcoded for testing with nestest.nes

    byte a{0x00};           // Accumalator
    byte x{0x00}, y{0x00};  // General purpose registers
    word pc{0xC000};        // Program counter
    byte s{0xFD};           // Stack pointer
    byte p{0x24};           // Status register

    std::shared_ptr<Bus> bus{};

    // Addressing Modes

    // Takes 2 cycles
    EffectiveAddress AbsoluteAddressing();
    // Takes 4 cycles
    EffectiveAddress IndirectAddressing();
    // Takes 1 cycle
    EffectiveAddress ZeroPageAddressing();
    // Takes 2 cycles
    EffectiveAddress ZeroPageXAddressing();
    // Takes 2 cycles
    EffectiveAddress ZeroPageYAddressing();
    // Takes 2 cycles; 3 if page crossed
    EffectiveAddress AbsoluteXIndexedAddressing();
    // Takes 2 cycles; 3 if page crossed
    EffectiveAddress AbsoluteYIndexedAddressing();
    // Takes 4 cycles
    EffectiveAddress IndirectXAddressing();
    // Takes 4 cycles; 5 if page crossed
    EffectiveAddress IndirectYAddressing();

    EffectiveAddress FetchEffectiveAddress(AddressingMode mode);

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
    void CLD(Opcode opcode);
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
    void JAM(Opcode opcode);
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
    void SED(Opcode opcode);
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

    // Opcode helpers
    void RelativeBranchOnCondition(bool condition);
    void CompareRegisterAndMemory(Opcode opcode, byte reg);

   public:
    Cpu() = default;

    Cpu(std::shared_ptr<Bus> bus) : bus{std::move(bus)} { spdlog::debug("Initialized CPU"); }

    bool IsSet(StatusFlag flag) const { return (p & static_cast<byte>(flag)) != 0; }

    void UpdateStatusFlag(StatusFlag flag, bool value) {
        if (value) {
            p |= static_cast<byte>(flag);
        } else {
            p &= ~static_cast<byte>(flag);
        }
    }

    // Runs the CPU startup procedure. Should run for 7 NES cycles
    void Start(){};

    byte Fetch() { return bus->CpuRead(pc++); }

    void Execute();
    void ExecuteOpcode(Opcode opcode);
};
