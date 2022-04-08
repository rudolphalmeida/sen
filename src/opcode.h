//
// Created by rudolph on 29/3/22.
//

#ifndef SEN_SRC_OPCODE_H_
#define SEN_SRC_OPCODE_H_

#include "utils.h"

enum class OpcodeClass {
    // Add Memory to Accumulator With Carry
    ADC,
    // AND Memory with Accumulator
    AND,
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
    OpcodeClass class_;
    byte opcode;
    AddressingMode mode;

    size_t length;
    cycles_t cycles;
};

Opcode DecodeOpcode(byte opcode);

#endif  // SEN_SRC_OPCODE_H_
