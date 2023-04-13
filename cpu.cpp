#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "constants.hxx"
#include "cpu.hxx"
#include "tests/flatbus.hxx"

#define OPCODE_CASE(opc)   \
    case OpcodeClass::opc: \
        opc(opcode);       \
        break;

inline word NonPageCrossingAdd(word value, word increment) {
    return (value & 0xFF00) | ((value + increment) & 0xFF);
}

std::array<Opcode, 256> OPCODES{
    {
        {OpcodeClass::BRK, 0x0, AddressingMode::Implied, 1, 7},
        {OpcodeClass::ORA, 0x1, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x2, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x4, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::ORA, 0x5, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::ASL, 0x6, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0x7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::PHP, 0x8, AddressingMode::Implied, 1, 3},
        {OpcodeClass::ORA, 0x9, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::ASL, 0xA, AddressingMode::Accumulator, 1, 2},
        {OpcodeClass::JAM, 0xB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0xC, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::ORA, 0xD, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::ASL, 0xE, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0xF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BPL, 0x10, AddressingMode::Relative, 2, 2},
        {OpcodeClass::ORA, 0x11, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0x12, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x13, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x14, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ORA, 0x15, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ASL, 0x16, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x17, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CLC, 0x18, AddressingMode::Implied, 1, 2},
        {OpcodeClass::ORA, 0x19, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::NOP, 0x1A, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x1B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x1C, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ORA, 0x1D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ASL, 0x1E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x1F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JSR, 0x20, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::AND, 0x21, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x22, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x23, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BIT, 0x24, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::AND, 0x25, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::ROL, 0x26, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0x27, AddressingMode::Implied, 1, 1},
        {OpcodeClass::PLP, 0x28, AddressingMode::Implied, 1, 4},
        {OpcodeClass::AND, 0x29, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::ROL, 0x2A, AddressingMode::Accumulator, 1, 2},
        {OpcodeClass::JAM, 0x2B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BIT, 0x2C, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::AND, 0x2D, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::ROL, 0x2E, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0x2F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BMI, 0x30, AddressingMode::Relative, 2, 2},
        {OpcodeClass::AND, 0x31, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0x32, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x33, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x34, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::AND, 0x35, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ROL, 0x36, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x37, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SEC, 0x38, AddressingMode::Implied, 1, 2},
        {OpcodeClass::AND, 0x39, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::NOP, 0x3A, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x3B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x3C, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::AND, 0x3D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ROL, 0x3E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x3F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::RTI, 0x40, AddressingMode::Implied, 1, 6},
        {OpcodeClass::EOR, 0x41, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x42, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x43, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x44, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::EOR, 0x45, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::LSR, 0x46, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0x47, AddressingMode::Implied, 1, 1},
        {OpcodeClass::PHA, 0x48, AddressingMode::Implied, 1, 3},
        {OpcodeClass::EOR, 0x49, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::LSR, 0x4A, AddressingMode::Accumulator, 1, 2},
        {OpcodeClass::JAM, 0x4B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JMP, 0x4C, AddressingMode::Absolute, 3, 3},
        {OpcodeClass::EOR, 0x4D, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::LSR, 0x4E, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0x4F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BVC, 0x50, AddressingMode::Relative, 2, 2},
        {OpcodeClass::EOR, 0x51, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0x52, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x53, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x54, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::EOR, 0x55, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::LSR, 0x56, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x57, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CLI, 0x58, AddressingMode::Implied, 1, 2},
        {OpcodeClass::EOR, 0x59, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::NOP, 0x5A, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x5B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x5C, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::EOR, 0x5D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::LSR, 0x5E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x5F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::RTS, 0x60, AddressingMode::Implied, 1, 6},
        {OpcodeClass::ADC, 0x61, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x62, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x63, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x64, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::ADC, 0x65, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::ROR, 0x66, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0x67, AddressingMode::Implied, 1, 1},
        {OpcodeClass::PLA, 0x68, AddressingMode::Implied, 1, 4},
        {OpcodeClass::ADC, 0x69, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::ROR, 0x6A, AddressingMode::Accumulator, 1, 2},
        {OpcodeClass::JAM, 0x6B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JMP, 0x6C, AddressingMode::Indirect, 3, 5},
        {OpcodeClass::ADC, 0x6D, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::ROR, 0x6E, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0x6F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BVS, 0x70, AddressingMode::Relative, 2, 2},
        {OpcodeClass::ADC, 0x71, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0x72, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x73, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x74, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ADC, 0x75, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ROR, 0x76, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x77, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SEI, 0x78, AddressingMode::Implied, 1, 2},
        {OpcodeClass::ADC, 0x79, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::NOP, 0x7A, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x7B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x7C, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ADC, 0x7D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ROR, 0x7E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x7F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0x80, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::STA, 0x81, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::NOP, 0x82, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::JAM, 0x83, AddressingMode::Implied, 1, 1},
        {OpcodeClass::STY, 0x84, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::STA, 0x85, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::STX, 0x86, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::JAM, 0x87, AddressingMode::Implied, 1, 1},
        {OpcodeClass::DEY, 0x88, AddressingMode::Implied, 1, 2},
        {OpcodeClass::NOP, 0x89, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::TXA, 0x8A, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x8B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::STY, 0x8C, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::STA, 0x8D, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::STX, 0x8E, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::JAM, 0x8F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BCC, 0x90, AddressingMode::Relative, 2, 2},
        {OpcodeClass::STA, 0x91, AddressingMode::IndirectY, 2, 6},
        {OpcodeClass::JAM, 0x92, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x93, AddressingMode::Implied, 1, 1},
        {OpcodeClass::STY, 0x94, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::STA, 0x95, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::STX, 0x96, AddressingMode::ZeroPageY, 2, 4},
        {OpcodeClass::JAM, 0x97, AddressingMode::Implied, 1, 1},
        {OpcodeClass::TYA, 0x98, AddressingMode::Implied, 1, 2},
        {OpcodeClass::STA, 0x99, AddressingMode::AbsoluteYIndexed, 3, 5},
        {OpcodeClass::TXS, 0x9A, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x9B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x9C, AddressingMode::Implied, 1, 1},
        {OpcodeClass::STA, 0x9D, AddressingMode::AbsoluteXIndexed, 3, 5},
        {OpcodeClass::JAM, 0x9E, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x9F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::LDY, 0xA0, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::LDA, 0xA1, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::LDX, 0xA2, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::JAM, 0xA3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::LDY, 0xA4, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::LDA, 0xA5, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::LDX, 0xA6, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::JAM, 0xA7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::TAY, 0xA8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::LDA, 0xA9, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::TAX, 0xAA, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0xAB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::LDY, 0xAC, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::LDA, 0xAD, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::LDX, 0xAE, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::JAM, 0xAF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BCS, 0xB0, AddressingMode::Relative, 2, 2},
        {OpcodeClass::LDA, 0xB1, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0xB2, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xB3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::LDY, 0xB4, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::LDA, 0xB5, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::LDX, 0xB6, AddressingMode::ZeroPageY, 2, 4},
        {OpcodeClass::JAM, 0xB7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CLV, 0xB8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::LDA, 0xB9, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::TSX, 0xBA, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0xBB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::LDY, 0xBC, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::LDA, 0xBD, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::LDX, 0xBE, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0xBF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPY, 0xC0, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::CMP, 0xC1, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::NOP, 0xC2, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::JAM, 0xC3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPY, 0xC4, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::CMP, 0xC5, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::DEC, 0xC6, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0xC7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::INY, 0xC8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::CMP, 0xC9, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::DEX, 0xCA, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0xCB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPY, 0xCC, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::CMP, 0xCD, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::DEC, 0xCE, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0xCF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BNE, 0xD0, AddressingMode::Relative, 2, 2},
        {OpcodeClass::CMP, 0xD1, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0xD2, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xD3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0xD4, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::CMP, 0xD5, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::DEC, 0xD6, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0xD7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CLD, 0xD8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::CMP, 0xD9, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::NOP, 0xDA, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0xDB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0xDC, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::CMP, 0xDD, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::DEC, 0xDE, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0xDF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPX, 0xE0, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::SBC, 0xE1, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::NOP, 0xE2, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::JAM, 0xE3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPX, 0xE4, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::SBC, 0xE5, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::INC, 0xE6, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0xE7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::INX, 0xE8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::SBC, 0xE9, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::NOP, 0xEA, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0xEB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPX, 0xEC, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::SBC, 0xED, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::INC, 0xEE, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0xEF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BEQ, 0xF0, AddressingMode::Relative, 2, 2},
        {OpcodeClass::SBC, 0xF1, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0xF2, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xF3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0xF4, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::SBC, 0xF5, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::INC, 0xF6, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0xF7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SED, 0xF8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::SBC, 0xF9, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::NOP, 0xFA, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0xFB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::NOP, 0xFC, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::SBC, 0xFD, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::INC, 0xFE, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0xFF, AddressingMode::Implied, 1, 1},
    },
};

template <typename BusType>
void Cpu<BusType>::Start() {
    // The CPU start procedure takes 7 NES cycles
    bus->CpuRead(0x0000);          // Address does not matter
    bus->CpuRead(0x0001);          // First start state
    bus->CpuRead(0x0100 + s);      // Second start state
    bus->CpuRead(0x0100 + s - 1);  // Third start state
    bus->CpuRead(0x0100 + s - 2);  // Fourth start state
    auto pcl = bus->CpuRead(RESET_VECTOR);
    auto pch = bus->CpuRead(RESET_VECTOR + 1);
    pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
    spdlog::info("Starting execution at {:#06X}", pc);
};

template <typename BusType>
void Cpu<BusType>::Execute() {
    CheckForInterrupts();

    auto opcode = OPCODES[Fetch()];
    ExecuteOpcode(opcode);
};

template <typename BusType>
void Cpu<BusType>::CheckForInterrupts() {
    if (*nmi_requested) {
        *nmi_requested = false;

        bus->CpuRead(pc);
        bus->CpuRead(pc);
        bus->CpuWrite(0x100 + s--, static_cast<byte>(pc >> 8));
        bus->CpuWrite(0x100 + s--, static_cast<byte>(pc));

        UpdateStatusFlag(StatusFlag::InterruptDisable, true);
        // Ensure the B flag is not set when pushing
        UpdateStatusFlag(StatusFlag::_B, false);
        p |= (1 << 5);  // The unused flag is set when pushing by NMI
        bus->CpuWrite(0x100 + s--, p);

        auto pcl = bus->CpuRead(NMI_VECTOR);
        auto pch = bus->CpuRead(NMI_VECTOR + 1);
        pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
    }
}

template <typename BusType>
void Cpu<BusType>::ExecuteOpcode(Opcode opcode) {
    switch (opcode.opcode_class) {
        OPCODE_CASE(ADC)
        OPCODE_CASE(AND)
        OPCODE_CASE(ASL)
        OPCODE_CASE(BCC)
        OPCODE_CASE(BCS)
        OPCODE_CASE(BEQ)
        OPCODE_CASE(BIT)
        OPCODE_CASE(BMI)
        OPCODE_CASE(BNE)
        OPCODE_CASE(BPL)
        OPCODE_CASE(BRK)
        OPCODE_CASE(BVC)
        OPCODE_CASE(BVS)
        OPCODE_CASE(CLC)
        OPCODE_CASE(CLD)
        OPCODE_CASE(CLI)
        OPCODE_CASE(CLV)
        OPCODE_CASE(CMP)
        OPCODE_CASE(CPX)
        OPCODE_CASE(CPY)
        OPCODE_CASE(DEC)
        OPCODE_CASE(DEX)
        OPCODE_CASE(DEY)
        OPCODE_CASE(EOR)
        OPCODE_CASE(INC)
        OPCODE_CASE(INX)
        OPCODE_CASE(INY)
        OPCODE_CASE(JAM)
        OPCODE_CASE(JMP)
        OPCODE_CASE(JSR)
        OPCODE_CASE(LDA)
        OPCODE_CASE(LDX)
        OPCODE_CASE(LDY)
        OPCODE_CASE(LSR)
        OPCODE_CASE(NOP)
        OPCODE_CASE(ORA)
        OPCODE_CASE(PHA)
        OPCODE_CASE(PHP)
        OPCODE_CASE(PLA)
        OPCODE_CASE(PLP)
        OPCODE_CASE(ROL)
        OPCODE_CASE(ROR)
        OPCODE_CASE(RTI)
        OPCODE_CASE(RTS)
        OPCODE_CASE(SBC)
        OPCODE_CASE(SEC)
        OPCODE_CASE(SED)
        OPCODE_CASE(SEI)
        OPCODE_CASE(STA)
        OPCODE_CASE(STX)
        OPCODE_CASE(STY)
        OPCODE_CASE(TAX)
        OPCODE_CASE(TAY)
        OPCODE_CASE(TSX)
        OPCODE_CASE(TXA)
        OPCODE_CASE(TXS)
        OPCODE_CASE(TYA)
        default:
            spdlog::error("Unimplemented opcode {:#04X} found", opcode.opcode);
            std::exit(-1);
    }
}

// Addressing Modes
template <typename BusType>
EffectiveAddress Cpu<BusType>::AbsoluteAddressing() {
    auto low = Fetch();
    auto high = Fetch();

    return {static_cast<word>(high) << 8 | static_cast<word>(low), false};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::IndirectAddressing() {
    auto [pointer, _] = AbsoluteAddressing();

    auto low = bus->CpuRead(pointer);
    auto high = bus->CpuRead(NonPageCrossingAdd(pointer, 1));

    return {static_cast<word>(high) << 8 | static_cast<word>(low), false};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::ZeroPageAddressing() {
    return {static_cast<word>(Fetch()), false};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::ZeroPageXAddressing() {
    auto [address, _] = ZeroPageAddressing();
    bus->Tick();  // Dummy read cycle
    return {NonPageCrossingAdd(address, x), false};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::ZeroPageYAddressing() {
    auto [address, _] = ZeroPageAddressing();
    bus->Tick();  // Dummy read cycle
    return {NonPageCrossingAdd(address, y), false};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::AbsoluteXIndexedAddressing() {
    auto [address, _] = AbsoluteAddressing();

    bool page_crossed = false;
    if ((address + x) != NonPageCrossingAdd(address, x)) {
        // If a page was crossed we require an additional cycle
        bus->Tick();  // Dummy read cycle
        page_crossed = true;
    }

    return {address + x, page_crossed};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::AbsoluteYIndexedAddressing() {
    auto [address, _] = AbsoluteAddressing();

    bool page_crossed = false;
    if ((address + y) != NonPageCrossingAdd(address, y)) {
        // If a page was crossed we require an additional cycle
        bus->Tick();  // Dummy read cycle
        page_crossed = true;
    }

    return {address + y, page_crossed};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::IndirectXAddressing() {
    auto operand = static_cast<word>(Fetch());
    auto pointer = operand + x;
    bus->CpuRead(operand);  // Dummy read cycle

    auto low = static_cast<word>(bus->CpuRead(pointer & 0xFF));
    auto high = static_cast<word>(bus->CpuRead((pointer + 1) & 0xFF));

    return {(high << 8) | low, false};
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::IndirectYAddressing() {
    auto pointer = static_cast<word>(Fetch());
    auto low = static_cast<word>(bus->CpuRead(pointer));
    auto high = static_cast<word>(bus->CpuRead((pointer + 1) & 0xFF));

    word effective = ((high << 8) | low);
    word non_page_crossed_address = NonPageCrossingAdd(effective, static_cast<word>(y));
    word page_crossed_address = effective + static_cast<word>(y);
    bus->CpuRead(non_page_crossed_address);

    if (non_page_crossed_address != page_crossed_address) {
        bus->CpuRead(page_crossed_address);  // Extra tick due to page crossing
        return {page_crossed_address, true};
    } else {
        return {non_page_crossed_address, false};
    }
}

template <typename BusType>
EffectiveAddress Cpu<BusType>::FetchEffectiveAddress(AddressingMode mode) {
    switch (mode) {
        case AddressingMode::Immediate:
            return {pc++, false};
        case AddressingMode::ZeroPage:
            return ZeroPageAddressing();
        case AddressingMode::ZeroPageX:
            return ZeroPageXAddressing();
        case AddressingMode::ZeroPageY:
            return ZeroPageYAddressing();
        case AddressingMode::Absolute:
            return AbsoluteAddressing();
        case AddressingMode::AbsoluteXIndexed:
            return AbsoluteXIndexedAddressing();
        case AddressingMode::AbsoluteYIndexed:
            return AbsoluteYIndexedAddressing();
        case AddressingMode::Indirect:
            return IndirectAddressing();
        case AddressingMode::IndirectX:
            return IndirectXAddressing();
        case AddressingMode::IndirectY:
            return IndirectYAddressing();
        default:
            spdlog::error("Invalid addressing mode for effective addresss");
            std::exit(-1);
    }
}

// Opcodes
template <typename BusType>
void Cpu<BusType>::BRK(Opcode) {
    Fetch();
    bus->CpuWrite(0x100 + s--, static_cast<byte>(pc >> 8));
    bus->CpuWrite(0x100 + s--, static_cast<byte>(pc));

    // NMI and IRQ can hijack BRK interrupt if they are pending here
    // TODO: Implement IRQ interrupt although it should not matter since both
    //       use the same vector
    auto interrupt_vector = *nmi_requested ? NMI_VECTOR : IRQ_VECTOR;
    byte temp_p = p | static_cast<byte>(StatusFlag::_B);  // Ensure bits 45 are set before push
    bus->CpuWrite(0x100 + s--, temp_p);
    UpdateStatusFlag(StatusFlag::InterruptDisable, true);

    auto pcl = bus->CpuRead(interrupt_vector);
    auto pch = bus->CpuRead(interrupt_vector + 1);
    pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
}

template <typename BusType>
void Cpu<BusType>::JMP(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    pc = address;
}

template <typename BusType>
void Cpu<BusType>::LDX(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    x = bus->CpuRead(address);
    UpdateStatusFlag(StatusFlag::Zero, x == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (x & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::STX(Opcode opcode) {
    auto [address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
         opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
        !page_crossed) {
        bus->CpuRead(address);  // Dummy read cycle
    }
    bus->CpuWrite(address, x);
}
template <typename BusType>
void Cpu<BusType>::JSR(Opcode) {
    auto low = static_cast<word>(Fetch());

    bus->CpuRead(0x100 + s);  // Dummy read cycle (3)

    bus->CpuWrite(0x100 + s--, static_cast<byte>(pc >> 8));
    bus->CpuWrite(0x100 + s--, static_cast<byte>(pc));

    word high = static_cast<word>(Fetch()) << 8;

    pc = high | low;
}

template <typename BusType>
void Cpu<BusType>::RTS(Opcode) {
    bus->CpuRead(pc);  // Fetch next opcode and discard it

    bus->CpuRead(0x100 + s++);  // Dummy read cycle (3)

    auto low = bus->CpuRead(0x100 + s++);
    auto high = bus->CpuRead(0x100 + s);
    pc = (high << 8) | low;

    bus->CpuRead(pc++);
}

template <typename BusType>
void Cpu<BusType>::NOP(Opcode opcode) {
    if (opcode.addressing_mode != AddressingMode::Implied) {
        auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
        bus->CpuRead(address);
    } else {
        bus->Tick();
    }
}

template <typename BusType>
void Cpu<BusType>::JAM(Opcode) {
    // These two reads are based on ProcessorTests
    bus->CpuRead(pc);
    bus->CpuRead(pc);
    pc--;  // The PC should not be incremented after a JAM opcode
}

template <typename BusType>
void Cpu<BusType>::SEC(Opcode) {
    UpdateStatusFlag(StatusFlag::Carry, true);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::CLC(Opcode) {
    UpdateStatusFlag(StatusFlag::Carry, false);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::CLD(Opcode) {
    UpdateStatusFlag(StatusFlag::Decimal, false);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::CLV(Opcode) {
    UpdateStatusFlag(StatusFlag::Overflow, false);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::CLI(Opcode) {
    UpdateStatusFlag(StatusFlag::InterruptDisable, false);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::SEI(Opcode) {
    UpdateStatusFlag(StatusFlag::InterruptDisable, true);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::SED(Opcode) {
    UpdateStatusFlag(StatusFlag::Decimal, true);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::BCC(Opcode) {
    RelativeBranchOnCondition(!IsSet(StatusFlag::Carry));
}

template <typename BusType>
void Cpu<BusType>::BCS(Opcode) {
    RelativeBranchOnCondition(IsSet(StatusFlag::Carry));
}

template <typename BusType>
void Cpu<BusType>::BEQ(Opcode) {
    RelativeBranchOnCondition(IsSet(StatusFlag::Zero));
}

template <typename BusType>
void Cpu<BusType>::BNE(Opcode) {
    RelativeBranchOnCondition(!IsSet(StatusFlag::Zero));
}

template <typename BusType>
void Cpu<BusType>::BMI(Opcode) {
    RelativeBranchOnCondition(IsSet(StatusFlag::Negative));
}

template <typename BusType>
void Cpu<BusType>::BPL(Opcode) {
    RelativeBranchOnCondition(!IsSet(StatusFlag::Negative));
}

template <typename BusType>
void Cpu<BusType>::BVC(Opcode) {
    RelativeBranchOnCondition(!IsSet(StatusFlag::Overflow));
}

template <typename BusType>
void Cpu<BusType>::BVS(Opcode) {
    RelativeBranchOnCondition(IsSet(StatusFlag::Overflow));
}

template <typename BusType>
void Cpu<BusType>::LDA(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    if (opcode.addressing_mode == AddressingMode::IndirectY) {
        a = bus->UntickedCpuRead(address);
    } else {
        a = bus->CpuRead(address);
    }
    UpdateStatusFlag(StatusFlag::Zero, a == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::STA(Opcode opcode) {
    auto [address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
         opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
        !page_crossed) {
        bus->CpuRead(address);  // Dummy read cycle
    }

    if (opcode.addressing_mode == AddressingMode::IndirectY && page_crossed) {
        bus->UntickedCpuWrite(address, a);
    } else {
        bus->CpuWrite(address, a);
    }
}

template <typename BusType>
void Cpu<BusType>::STY(Opcode opcode) {
    auto [address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
         opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
        !page_crossed) {
        bus->CpuRead(address);  // Dummy read cycle
    }
    bus->CpuWrite(address, y);
}

template <typename BusType>
void Cpu<BusType>::BIT(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    auto operand = bus->CpuRead(address);

    UpdateStatusFlag(StatusFlag::Negative, (operand & 0x80) != 0x00);
    UpdateStatusFlag(StatusFlag::Overflow, (operand & 0x40) != 0x00);
    UpdateStatusFlag(StatusFlag::Zero, (operand & a) == 0x00);
}

template <typename BusType>
void Cpu<BusType>::PHA(Opcode) {
    bus->CpuRead(pc);  // Fetch next opcode and discard it
    bus->CpuWrite(0x100 + s--, a);
}

template <typename BusType>
void Cpu<BusType>::PLA(Opcode) {
    bus->CpuRead(pc);           // Fetch next opcode and discard it
    bus->CpuRead(0x100 + s++);  // Dummy read cycle (3)
    a = bus->CpuRead(0x100 + s);
    UpdateStatusFlag(StatusFlag::Zero, a == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::PHP(Opcode) {
    bus->CpuRead(pc);                                     // Fetch next opcode and discard it
    byte temp_p = p | static_cast<byte>(StatusFlag::_B);  // Ensure bits 45 are set before push
    bus->CpuWrite(0x100 + s--, temp_p);
}

template <typename BusType>
void Cpu<BusType>::PLP(Opcode) {
    bus->CpuRead(pc);           // Fetch next opcode and discard it
    bus->CpuRead(0x100 + s++);  // Dummy read cycle (3)
    auto temp_p = bus->CpuRead(0x100 + s);
    // Bits 54 of the popped value from the stack should be ignored
    p = (p & 0x30) | (temp_p & 0xCF);
}

template <typename BusType>
void Cpu<BusType>::AND(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    byte operand{};
    if (opcode.addressing_mode == AddressingMode::IndirectY) {
        operand = bus->UntickedCpuRead(address);
    } else {
        operand = bus->CpuRead(address);
    }
    a = a & operand;
    UpdateStatusFlag(StatusFlag::Zero, a == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::ORA(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    byte operand{};
    if (opcode.addressing_mode == AddressingMode::IndirectY) {
        operand = bus->UntickedCpuRead(address);
    } else {
        operand = bus->CpuRead(address);
    }
    a = a | operand;
    UpdateStatusFlag(StatusFlag::Zero, a == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::EOR(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    byte operand{};
    if (opcode.addressing_mode == AddressingMode::IndirectY) {
        operand = bus->UntickedCpuRead(address);
    } else {
        operand = bus->CpuRead(address);
    }
    a = a ^ operand;
    UpdateStatusFlag(StatusFlag::Zero, a == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::CMP(Opcode opcode) {
    CompareRegisterAndMemory(opcode, a);
}

template <typename BusType>
void Cpu<BusType>::CPX(Opcode opcode) {
    CompareRegisterAndMemory(opcode, x);
}

template <typename BusType>
void Cpu<BusType>::CPY(Opcode opcode) {
    CompareRegisterAndMemory(opcode, y);
}

template <typename BusType>
void Cpu<BusType>::ADC(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    word operand{};
    if (opcode.addressing_mode == AddressingMode::IndirectY) {
        operand = static_cast<word>(bus->UntickedCpuRead(address));
    } else {
        operand = static_cast<word>(bus->CpuRead(address));
    }

    auto temp_a = static_cast<word>(a);
    word result = temp_a + operand + static_cast<word>(p & static_cast<byte>(StatusFlag::Carry));

    UpdateStatusFlag(StatusFlag::Zero, (result & 0xFF) == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (result & 0x80) != 0x00);
    UpdateStatusFlag(StatusFlag::Carry, result > 0xFF);
    // Reference:
    // https://github.com/OneLoneCoder/olcNES/blob/663e3777191c011135dfb6d40c887ae126970dd7/Part%20%233%20-%20Buses%2C%20Rams%2C%20Roms%20%26%20Mappers/olc6502.cpp#L589
    // http://www.6502.org/tutorials/vflag.html
    UpdateStatusFlag(StatusFlag::Overflow,
                     ((~(temp_a ^ operand) & (temp_a ^ result)) & 0x80) != 0x00);

    a = static_cast<byte>(result & 0xFF);
}

template <typename BusType>
void Cpu<BusType>::SBC(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    // Fetch operand and invert bottom 8 bits
    word operand{};
    if (opcode.addressing_mode == AddressingMode::IndirectY) {
        operand = static_cast<word>(bus->UntickedCpuRead(address));
    } else {
        operand = static_cast<word>(bus->CpuRead(address));
    }
    operand ^= 0x00FF;

    auto temp_a = static_cast<word>(a);
    word result = temp_a + operand + static_cast<word>(p & static_cast<byte>(StatusFlag::Carry));

    UpdateStatusFlag(StatusFlag::Zero, (result & 0xFF) == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (result & 0x80) != 0x00);
    UpdateStatusFlag(StatusFlag::Carry, result > 0xFF);
    // Reference:
    // https://github.com/OneLoneCoder/olcNES/blob/663e3777191c011135dfb6d40c887ae126970dd7/Part%20%233%20-%20Buses%2C%20Rams%2C%20Roms%20%26%20Mappers/olc6502.cpp#L589
    // http://www.6502.org/tutorials/vflag.html
    UpdateStatusFlag(StatusFlag::Overflow,
                     ((~(temp_a ^ operand) & (temp_a ^ result)) & 0x80) != 0x00);

    a = static_cast<byte>(result);
}

template <typename BusType>
void Cpu<BusType>::LDY(Opcode opcode) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    y = bus->CpuRead(address);
    UpdateStatusFlag(StatusFlag::Zero, y == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (y & 0x80) != 0x00);
}

template <typename BusType>
void Cpu<BusType>::INY(Opcode) {
    y++;
    UpdateStatusFlag(StatusFlag::Zero, y == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::INX(Opcode) {
    x++;
    UpdateStatusFlag(StatusFlag::Zero, x == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::INC(Opcode opcode) {
    auto [address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
         opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
        !page_crossed) {
        bus->CpuRead(address);  // Dummy read cycle
    }
    byte operand = bus->CpuRead(address);

    bus->CpuWrite(address, operand);  // Dummy write cycle
    operand++;
    UpdateStatusFlag(StatusFlag::Zero, operand == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (operand & 0x80) != 0x00);

    bus->CpuWrite(address, operand);
}

template <typename BusType>
void Cpu<BusType>::DEC(Opcode opcode) {
    auto [address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
         opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
        !page_crossed) {
        bus->CpuRead(address);  // Dummy read cycle
    }
    byte operand = bus->CpuRead(address);

    bus->CpuWrite(address, operand);  // Dummy write cycle
    operand--;
    UpdateStatusFlag(StatusFlag::Zero, operand == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (operand & 0x80) != 0x00);

    bus->CpuWrite(address, operand);
}

template <typename BusType>
void Cpu<BusType>::DEY(Opcode) {
    y--;
    UpdateStatusFlag(StatusFlag::Zero, y == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::DEX(Opcode) {
    x--;
    UpdateStatusFlag(StatusFlag::Zero, x == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::TAX(Opcode) {
    x = a;
    UpdateStatusFlag(StatusFlag::Zero, x == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::TAY(Opcode) {
    y = a;
    UpdateStatusFlag(StatusFlag::Zero, y == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::TSX(Opcode) {
    x = s;
    UpdateStatusFlag(StatusFlag::Zero, x == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::TXA(Opcode) {
    a = x;
    UpdateStatusFlag(StatusFlag::Zero, a == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (a & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::TXS(Opcode) {
    s = x;
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::TYA(Opcode) {
    a = y;
    UpdateStatusFlag(StatusFlag::Zero, y == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->Tick();
}

template <typename BusType>
void Cpu<BusType>::RTI(Opcode) {
    bus->CpuRead(pc);             // Fetch next opcode and discard it
    bus->CpuRead(0x100 + (++s));  // Dummy read cycle
    auto temp_p = bus->CpuRead(0x100 + s++);
    // Bits 54 of the popped value from the stack should be ignored
    p = (p & 0x30) | (temp_p & 0xCF);

    auto low = static_cast<word>(bus->CpuRead(0x100 + s++));
    auto high = static_cast<word>(bus->CpuRead(0x100 + s));

    pc = (high << 8) | low;
}

template <typename BusType>
void Cpu<BusType>::LSR(Opcode opcode) {
    byte operand{};
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
             opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
            !page_crossed) {
            bus->CpuRead(address);  // Dummy read cycle
        }
        operand = bus->CpuRead(address);
    }

    byte result = operand >> 1;
    UpdateStatusFlag(StatusFlag::Negative, false);
    UpdateStatusFlag(StatusFlag::Zero, result == 0x00);
    UpdateStatusFlag(StatusFlag::Carry, (operand & 0b1) != 0x00);
    bus->Tick();  // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->CpuWrite(address, result);
    }
}

template <typename BusType>
void Cpu<BusType>::ASL(Opcode opcode) {
    byte operand{};
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
             opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
            !page_crossed) {
            bus->CpuRead(address);  // Dummy read cycle
        }
        operand = bus->CpuRead(address);
    }

    byte result = operand << 1;
    UpdateStatusFlag(StatusFlag::Negative, (result & 0x80) != 0);
    UpdateStatusFlag(StatusFlag::Zero, result == 0x00);
    UpdateStatusFlag(StatusFlag::Carry, (operand & 0x80) != 0x00);
    bus->Tick();  // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->CpuWrite(address, result);
    }
}

template <typename BusType>
void Cpu<BusType>::ROL(Opcode opcode) {
    byte operand{};
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
             opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
            !page_crossed) {
            bus->CpuRead(address);  // Dummy read cycle
        }
        operand = bus->CpuRead(address);
    }

    byte result = (operand << 1) | (IsSet(StatusFlag::Carry) ? 0b1 : 0b0);
    UpdateStatusFlag(StatusFlag::Negative, (result & 0x80) != 0);
    UpdateStatusFlag(StatusFlag::Zero, result == 0x00);
    UpdateStatusFlag(StatusFlag::Carry, (operand & 0x80) != 0x00);
    bus->Tick();  // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->CpuWrite(address, result);
    }
}

template <typename BusType>
void Cpu<BusType>::ROR(Opcode opcode) {
    byte operand{};
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = FetchEffectiveAddress(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed ||
             opcode.addressing_mode == AddressingMode::AbsoluteYIndexed) &&
            !page_crossed) {
            bus->CpuRead(address);  // Dummy read cycle
        }
        operand = bus->CpuRead(address);
    }

    byte result = (operand >> 1) | (IsSet(StatusFlag::Carry) ? 0x80 : 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (result & 0x80) != 0);
    UpdateStatusFlag(StatusFlag::Zero, result == 0x00);
    UpdateStatusFlag(StatusFlag::Carry, (operand & 0b1) != 0x00);
    bus->Tick();  // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->CpuWrite(address, result);
    }
}

// Opcode helpers

template <typename BusType>
void Cpu<BusType>::RelativeBranchOnCondition(bool condition) {
    // First change the type, then the size, then type again
    auto offset = (word)(int16_t)(int8_t)Fetch();

    if (!condition)
        return;

    bus->Tick();  // Cycle 3 if branch taken

    word page_crossed = pc + offset;
    word non_page_crossed = NonPageCrossingAdd(pc, offset);

    pc = non_page_crossed;

    if (page_crossed != non_page_crossed) {
        bus->Tick();  // Cycle 4 for page crossing
        pc = page_crossed;
    }
}

template <typename BusType>
void Cpu<BusType>::CompareRegisterAndMemory(Opcode opcode, byte reg) {
    auto [address, _] = FetchEffectiveAddress(opcode.addressing_mode);
    word operand{};
    if (opcode.opcode_class == OpcodeClass::CMP &&
        opcode.addressing_mode == AddressingMode::IndirectY) {
        operand = static_cast<word>(bus->UntickedCpuRead(address));
    } else {
        operand = static_cast<word>(bus->CpuRead(address));
    }

    byte result = reg - operand;
    UpdateStatusFlag(StatusFlag::Zero, result == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (result & 0x80) != 0x00);
    // The carry flag is set if no borrow or greater than or equal for A - M
    UpdateStatusFlag(StatusFlag::Carry, reg >= operand);
}

template class Cpu<Bus>;
template class Cpu<FlatBus>;