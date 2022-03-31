//
// Created by rudolph on 29/3/22.
//

#ifndef SEN_SRC_OPCODE_H_
#define SEN_SRC_OPCODE_H_

#include "utils.h"

enum class OpcodeClass {
    JMP,
    JAM,  // Not really an opcode, jams the CPU when executed
    LDX,
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
