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
    BNE,  // Branch on Result Not Zero
    BPL,  // Branch on Result Plus
    BVC,  // Branch on Overflow Clear
    BVS,  // Branch on Overflow Set
    CLC,  // Clear Carry Flag
    CMP,  // Compare Memory With Accumulator
    JAM,  // Not really an opcode, jams the CPU when executed
    JMP,  // Jump
    JSR,  // Jump, Saving Return Address
    LDA,  // Load Accumulator with Memory
    LDX,  // Load X from Memory
    NOP,  // No Operation
    PLA,  // Pull Accumulator From Stack
    PHP,  // Push Processor Status on Stack
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
