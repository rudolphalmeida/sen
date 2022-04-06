//
// Created by rudolph on 29/3/22.
//

#ifndef SEN_SRC_OPCODE_H_
#define SEN_SRC_OPCODE_H_

#include "utils.h"

enum class OpcodeClass {
    AND,  // AND Memory with Accumulator
    BCC,  // Branch on Carry Clear
    BCS,  // Branch on Carry Set
    BEQ,  // Branch on Result Zero
    BIT,  // Test Bits in Memory with Accumulator
    BMI,  // Branch on Resul Minus
    BNE,  // Branch on Result Not Zero
    BPL,  // Branch on Result Plus
    BVC,  // Branch on Overflow Clear
    BVS,  // Branch on Overflow Set
    CLC,  // Clear Carry Flag
    CLD,  // Clear Decimal Mode
    CLV,  // Clear Overflow Bug
    CMP,  // Compare Memory With Accumulator
    EOR,  // Exclusive-OR Memory with Accumulator
    JAM,  // Not really an opcode, jams the CPU when executed
    JMP,  // Jump
    JSR,  // Jump, Saving Return Address
    LDA,  // Load Accumulator with Memory
    LDX,  // Load X from Memory
    NOP,  // No Operation
    ORA,  // OR Memory with Accumulator
    PHA,  // Push Accumulator To Stack
    PHP,  // Push Processor Status on Stack
    PLA,  // Pull Accumulator From Stack
    PLP,  // Pull Processor Status From Stack
    RTS,  // Return from Subroutine
    SEC,  // Set Carry Flag
    SED,  // Set Decimal Flag
    SEI,  // Set Interrupt Disable Status
    STA,  // Store Accumulator in Memory
    STX,  // Store X to Memory
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
    OpcodeClass class_;
    byte opcode;
    AddressingMode mode;

    size_t length;
    cycles_t cycles;
};

Opcode DecodeOpcode(byte opcode);

#endif  // SEN_SRC_OPCODE_H_
