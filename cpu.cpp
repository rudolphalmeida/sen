#include <cstdint>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "constants.hxx"
#include "cpu.hxx"

#define OPCODE_CASE(opc)   \
    case OpcodeClass::opc: \
        opc(opcode);       \
        break;

#define NOT_SUPPORTED_ADDRESSING_MODE(opcode)                                           \
    default:                                                                            \
        spdlog::error("Unsupported addressing mode for opcode {:#04X}", opcode.opcode); \
        std::exit(-1);

inline word NonPageCrossingAdd(word value, word increment) {
    return (value & 0xFF00) | ((value + increment) & 0xFF);
}

std::array<Opcode, 256> OPCODES{
    {
        {OpcodeClass::JAM, 0x0, AddressingMode::Implied, 1, 1},
        {OpcodeClass::ORA, 0x1, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x2, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x3, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x4, AddressingMode::Implied, 1, 1},
        {OpcodeClass::ORA, 0x5, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::ASL, 0x6, AddressingMode::ZeroPage, 2, 5},
        {OpcodeClass::JAM, 0x7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::PHP, 0x8, AddressingMode::Implied, 1, 3},
        {OpcodeClass::ORA, 0x9, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::ASL, 0xA, AddressingMode::Accumulator, 1, 2},
        {OpcodeClass::JAM, 0xB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xC, AddressingMode::Implied, 1, 1},
        {OpcodeClass::ORA, 0xD, AddressingMode::Absolute, 3, 4},
        {OpcodeClass::ASL, 0xE, AddressingMode::Absolute, 3, 6},
        {OpcodeClass::JAM, 0xF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::BPL, 0x10, AddressingMode::Relative, 2, 2},
        {OpcodeClass::ORA, 0x11, AddressingMode::IndirectY, 2, 5},
        {OpcodeClass::JAM, 0x12, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x13, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x14, AddressingMode::Implied, 1, 1},
        {OpcodeClass::ORA, 0x15, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ASL, 0x16, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x17, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CLC, 0x18, AddressingMode::Implied, 1, 2},
        {OpcodeClass::ORA, 0x19, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0x1A, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x1B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x1C, AddressingMode::Implied, 1, 1},
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
        {OpcodeClass::JAM, 0x34, AddressingMode::Implied, 1, 1},
        {OpcodeClass::AND, 0x35, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ROL, 0x36, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x37, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SEC, 0x38, AddressingMode::Implied, 1, 2},
        {OpcodeClass::AND, 0x39, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0x3A, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x3B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x3C, AddressingMode::Implied, 1, 1},
        {OpcodeClass::AND, 0x3D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ROL, 0x3E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x3F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::RTI, 0x40, AddressingMode::Implied, 1, 6},
        {OpcodeClass::EOR, 0x41, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x42, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x43, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x44, AddressingMode::Implied, 1, 1},
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
        {OpcodeClass::JAM, 0x54, AddressingMode::Implied, 1, 1},
        {OpcodeClass::EOR, 0x55, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::LSR, 0x56, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x57, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x58, AddressingMode::Implied, 1, 1},
        {OpcodeClass::EOR, 0x59, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0x5A, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x5B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x5C, AddressingMode::Implied, 1, 1},
        {OpcodeClass::EOR, 0x5D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::LSR, 0x5E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x5F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::RTS, 0x60, AddressingMode::Implied, 1, 6},
        {OpcodeClass::ADC, 0x61, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x62, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x63, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x64, AddressingMode::Implied, 1, 1},
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
        {OpcodeClass::JAM, 0x74, AddressingMode::Implied, 1, 1},
        {OpcodeClass::ADC, 0x75, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::ROR, 0x76, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0x77, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SEI, 0x78, AddressingMode::Implied, 1, 2},
        {OpcodeClass::ADC, 0x79, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0x7A, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x7B, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x7C, AddressingMode::Implied, 1, 1},
        {OpcodeClass::ADC, 0x7D, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::ROR, 0x7E, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0x7F, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x80, AddressingMode::Implied, 1, 1},
        {OpcodeClass::STA, 0x81, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0x82, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0x83, AddressingMode::Implied, 1, 1},
        {OpcodeClass::STY, 0x84, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::STA, 0x85, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::STX, 0x86, AddressingMode::ZeroPage, 2, 3},
        {OpcodeClass::JAM, 0x87, AddressingMode::Implied, 1, 1},
        {OpcodeClass::DEY, 0x88, AddressingMode::Implied, 1, 2},
        {OpcodeClass::JAM, 0x89, AddressingMode::Implied, 1, 1},
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
        {OpcodeClass::JAM, 0xC2, AddressingMode::Implied, 1, 1},
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
        {OpcodeClass::JAM, 0xD4, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CMP, 0xD5, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::DEC, 0xD6, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0xD7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CLD, 0xD8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::CMP, 0xD9, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0xDA, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xDB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xDC, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CMP, 0xDD, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::DEC, 0xDE, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0xDF, AddressingMode::Implied, 1, 1},
        {OpcodeClass::CPX, 0xE0, AddressingMode::Immediate, 2, 2},
        {OpcodeClass::SBC, 0xE1, AddressingMode::IndirectX, 2, 6},
        {OpcodeClass::JAM, 0xE2, AddressingMode::Implied, 1, 1},
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
        {OpcodeClass::JAM, 0xF4, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SBC, 0xF5, AddressingMode::ZeroPageX, 2, 4},
        {OpcodeClass::INC, 0xF6, AddressingMode::ZeroPageX, 2, 6},
        {OpcodeClass::JAM, 0xF7, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SED, 0xF8, AddressingMode::Implied, 1, 2},
        {OpcodeClass::SBC, 0xF9, AddressingMode::AbsoluteYIndexed, 3, 4},
        {OpcodeClass::JAM, 0xFA, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xFB, AddressingMode::Implied, 1, 1},
        {OpcodeClass::JAM, 0xFC, AddressingMode::Implied, 1, 1},
        {OpcodeClass::SBC, 0xFD, AddressingMode::AbsoluteXIndexed, 3, 4},
        {OpcodeClass::INC, 0xFE, AddressingMode::AbsoluteXIndexed, 3, 7},
        {OpcodeClass::JAM, 0xFF, AddressingMode::Implied, 1, 1},
    },
};

void Cpu::Execute() {
    auto opcode = OPCODES[Fetch()];

    // Print the PC and CYC count from before the previous Fetch()
    fmt::print("{:04X}  {:02X} A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} CYC:{}\n", pc - 1,
               opcode.opcode, a, x, y, p, s, bus->cycles - 1);
    ExecuteOpcode(opcode);
};

void Cpu::ExecuteOpcode(Opcode opcode) {
    switch (opcode.opcode_class) {
        OPCODE_CASE(BCC)
        OPCODE_CASE(BCS)
        OPCODE_CASE(BEQ)
        OPCODE_CASE(BMI)
        OPCODE_CASE(BNE)
        OPCODE_CASE(BPL)
        OPCODE_CASE(BVC)
        OPCODE_CASE(BVS)
        OPCODE_CASE(CLC)
        OPCODE_CASE(JMP)
        OPCODE_CASE(JSR)
        OPCODE_CASE(LDX)
        OPCODE_CASE(NOP)
        OPCODE_CASE(SEC)
        OPCODE_CASE(STX)
        default:
            spdlog::error("Unimplemented opcode {:#04X} found", opcode.opcode);
            std::exit(-1);
    }
}

// Addressing Modes

word Cpu::AbsoluteAddressing() {
    auto low = Fetch();
    auto high = Fetch();

    return static_cast<word>(high) << 8 | static_cast<word>(low);
}

word Cpu::IndirectAddressing() {
    auto pointer = AbsoluteAddressing();

    auto low = bus->CpuRead(pointer);
    auto high = bus->CpuRead(NonPageCrossingAdd(pointer, 1));

    return static_cast<word>(high) << 8 | static_cast<word>(low);
}

word Cpu::ZeroPageAddressing() {
    return static_cast<word>(Fetch());
}

word Cpu::ZeroPageYAddressing() {
    auto address = ZeroPageAddressing();
    bus->Tick();  // Dummy read cycle
    return NonPageCrossingAdd(address, y);
}

word Cpu::AbsoluteYIndexedAddressing() {
    auto address = AbsoluteAddressing();

    if ((address + y) != NonPageCrossingAdd(address, y)) {
        // If a page was crossed we require an additional cycle
        bus->Tick();  // Dummy read cycle
    }

    return address + y;
}

// Opcodes

void Cpu::JMP(Opcode opcode) {
    word address{};
    switch (opcode.addressing_mode) {
        case AddressingMode::Absolute:
            address = AbsoluteAddressing();
            break;
        case AddressingMode::Indirect:
            address = IndirectAddressing();
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode)
    }

    pc = address;
}

void Cpu::LDX(Opcode opcode) {
    word address{};
    switch (opcode.addressing_mode) {
        case AddressingMode::Immediate:
            address = pc++;
            break;
        case AddressingMode::ZeroPage:
            address = ZeroPageAddressing();
            break;
        case AddressingMode::ZeroPageY:
            address = ZeroPageYAddressing();
            break;
        case AddressingMode::Absolute:
            address = AbsoluteAddressing();
            break;
        case AddressingMode::AbsoluteYIndexed:
            address = AbsoluteYIndexedAddressing();
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode)
    }

    x = bus->CpuRead(address);

    UpdateStatusFlag(StatusFlag::Zero, x == 0x00);
    UpdateStatusFlag(StatusFlag::Negative, (x & 0x80) != 0);
}

void Cpu::STX(Opcode opcode) {
    word address{};
    switch (opcode.addressing_mode) {
        case AddressingMode::ZeroPage:
            address = ZeroPageAddressing();
            break;
        case AddressingMode::ZeroPageY:
            address = ZeroPageYAddressing();
            break;
        case AddressingMode::Absolute:
            address = AbsoluteAddressing();
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode)
    }

    bus->CpuWrite(address, x);
}

void Cpu::JSR(Opcode opcode) {
    auto low = static_cast<word>(Fetch());

    bus->Tick();  // Dummy read cycle (3)

    bus->CpuWrite(0x100 + s--, static_cast<byte>(pc >> 8));
    bus->CpuWrite(0x100 + s--, static_cast<byte>(pc));

    auto high = static_cast<word>(Fetch()) << 8;

    pc = high | low;
}

void Cpu::NOP(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Implied:
            bus->Tick();  // Dummy read
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode)
    }
}

void Cpu::SEC(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Implied:
            bus->Tick();  // Dummy read
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode)
    }

    UpdateStatusFlag(StatusFlag::Carry, true);
}

void Cpu::CLC(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Implied:
            bus->Tick();  // Dummy read
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode)
    }

    UpdateStatusFlag(StatusFlag::Carry, false);
}

void Cpu::BCC(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(!IsSet(StatusFlag::Carry));
}

void Cpu::BCS(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(IsSet(StatusFlag::Carry));
}

void Cpu::BEQ(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(IsSet(StatusFlag::Zero));
}

void Cpu::BNE(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(!IsSet(StatusFlag::Zero));
}

void Cpu::BMI(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(IsSet(StatusFlag::Negative));
}

void Cpu::BPL(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(!IsSet(StatusFlag::Negative));
}

void Cpu::BVC(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(!IsSet(StatusFlag::Overflow));
}

void Cpu::BVS(Opcode opcode) {
    switch (opcode.addressing_mode) {
        case AddressingMode::Relative:
            break;

            NOT_SUPPORTED_ADDRESSING_MODE(opcode);
    }

    RelativeBranchOnCondition(IsSet(StatusFlag::Overflow));
}

// Opcode helpers
void Cpu::RelativeBranchOnCondition(bool condition) {
    // First change the type, then the size, then type again
    auto offset = (word)(int8_t)Fetch();

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
