#pragma once

#include <cassert>
#include <memory>

#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "constants.hxx"

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
    _Decimal = (1 << 3),          // Unused in NES
    _B = 0x30,                    // No CPU effect, bits 45
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
    word AbsoluteAddressing();
    // Takes 4 cycles
    word IndirectAddressing();
    // Takes 1 cycle
    word ZeroPageAddressing();
    // Takes 2 cycles
    word ZeroPageXAddressing();
    // Takes 2 cycles
    word ZeroPageYAddressing();
    // Takes 2 cycles; 3 if page crossed
    word AbsoluteXIndexedAddressing();
    // Takes 2 cycles; 3 if page crossed
    word AbsoluteYIndexedAddressing();
    // Takes 2 cycles
    word IndirectXAddressing();
    // Takes 4 cycles; 5 if page crossed
    word IndirectYAddressing();

    word EffectiveAddress(AddressingMode mode);

    // Opcodes
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
    void JMP(Opcode opcode);
    void JSR(Opcode opcode);
    void LDA(Opcode opcode);
    void LDX(Opcode opcode);
    void NOP(Opcode opcode);
    void RTS(Opcode opcode);
    void SEC(Opcode opcode);
    void STA(Opcode opcode);
    void STX(Opcode opcode);

    // Opcode helpers
    void RelativeBranchOnCondition(bool condition);

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
