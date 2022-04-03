//
// Created by rudolph on 20/3/22.
//

#include <array>

#include <fmt/format.h>

#include "cpu.h"

#define OPCODE_CASE(opc)   \
    case OpcodeClass::opc: \
        opc(opcode);       \
        break;

Cpu::Cpu(std::shared_ptr<Mmu> mmu) : mmu(std::move(mmu)) {
    // Set Power-Up State. Reference:
    // https://www.nesdev.org/wiki/CPU_power_up_state
    a = x = y = 0x00;
    p.value = 0x24;  // Differs from nesdev to match with nestest.nes
    s = 0xFD;

    cyc = 7;

    // TODO: Determine this by reading the reset vector. For nestest
    //       hardcoded to 0xC000
    pc = 0xC000;
}

void Cpu::Tick() {
    auto opcode_byte = Fetch();
    auto opcode = DecodeOpcode(opcode_byte);

    spdlog::debug(
        "{:4X}  {:02X} A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} CYC:{}",
        pc - 1, opcode_byte, a, x, y, p.value, s, cyc);

    ExecuteOpcode(opcode);

    cyc += opcode.cycles;
}

void Cpu::ExecuteOpcode(Opcode opcode) {
    switch (opcode.class_) {
        OPCODE_CASE(JMP)
        OPCODE_CASE(LDX)
        OPCODE_CASE(STX)
        OPCODE_CASE(JSR)
        OPCODE_CASE(LDA)
        OPCODE_CASE(NOP)
        OPCODE_CASE(SED)
        OPCODE_CASE(SEI)
        OPCODE_CASE(SEC)
        OPCODE_CASE(BCS)
        OPCODE_CASE(CLC)
        OPCODE_CASE(BCC)
        OPCODE_CASE(BEQ)
        OPCODE_CASE(BNE)
        OPCODE_CASE(STA)
        case OpcodeClass::JAM:
            spdlog::error("Unimplemented or Unknown opcode {:#4X} at {:#6X}",
                          opcode.opcode, pc - 1);
            std::exit(-1);
    }
}

byte Cpu::Fetch() {
    auto value = mmu->Read(pc++);
    return value;
}

// Addressing Modes
word Cpu::Indirect(word pointer) {
    auto latch = mmu->Read(pointer);

    word result = (word)(mmu->Read(pointer + 1)) << 8;
    result |= latch;

    return result;
}

word Cpu::ZeroPageIndexed(byte operand, byte offset) {
    mmu->Read((word)operand);
    return (word)(operand + offset);
}

word Cpu::Absolute() {
    word address = Fetch();         // Fetch low byte
    address |= (word)Fetch() << 8;  // Fetch high byte

    return address;
}

word Cpu::AbsoluteIndexed(byte offset) {
    auto low = Fetch();
    auto high = Fetch();
    word supplied = joinWord(high, low + offset);
    word corrected = joinWord(high, low) + offset;

    mmu->Read(supplied);
    auto effective = supplied;

    if (supplied != corrected) {
        effective = corrected;
        cyc += 1;
    }

    return effective;
}

word Cpu::IndirectX() {
    auto pointer = Fetch();

    mmu->Read((word)pointer);  // Dummy read
    pointer += x;

    word effective = (word)mmu->Read(pointer);
    effective |= ((word)mmu->Read(pointer + 1)) << 8;

    return effective;
}

word Cpu::IndirectY() {
    auto pointer = Fetch();

    auto low = mmu->Read((word)pointer);
    auto high = mmu->Read((word)(pointer + 1));

    auto supplied = joinWord(high, low + y);
    auto corrected = joinWord(high, low) + y;

    mmu->Read(supplied);
    auto effective = supplied;

    if (supplied != corrected) {
        effective = corrected;
        cyc += 1;
    }

    return effective;
}

// Opcodes
void Cpu::JMP(Opcode opcode) {
    word address = Absolute();

    switch (opcode.mode) {
        case AddressingMode::Absolute:
            pc = address;
            break;
        case AddressingMode::Indirect: {
            pc = Indirect(address);
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for JMP");
            std::exit(-1);
    }
}

void Cpu::LDX(Opcode opcode) {
    switch (opcode.mode) {
        case AddressingMode::Immediate:
            x = Fetch();
            break;
        case AddressingMode::ZeroPage: {
            auto address = (word)Fetch();  // Address falls on the zero page
            x = mmu->Read(address);
            break;
        }
        case AddressingMode::ZeroPageY: {
            auto address = ZeroPageIndexed(Fetch(), y);
            x = mmu->Read(address);
            break;
        }
        case AddressingMode::Absolute: {
            x = mmu->Read(Absolute());
            break;
        }
        case AddressingMode::AbsoluteYIndexed: {
            auto address = AbsoluteIndexed(y);
            x = mmu->Read(address);
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for LDX");
            std::exit(-1);
    }

    p.flags.Zero = x == 0;
    p.flags.Negative = isBitSet(x, 7);
}

void Cpu::STX(Opcode opcode) {
    word address;
    if (opcode.mode == AddressingMode::Absolute) {
        address = Absolute();
    } else {
        switch (opcode.mode) {
            case AddressingMode::ZeroPage: {
                address = (word)Fetch();
                break;
            }
            case AddressingMode::ZeroPageY: {
                address = ZeroPageIndexed(Fetch(), y);
                break;
            }
            default:
                spdlog::error("Invalid addressing mode for STX");
                std::exit(-1);
        }
    }

    mmu->Write(address, x);
}

void Cpu::JSR(Opcode opcode) {
    if (opcode.mode != AddressingMode::Absolute) {
        spdlog::error("Invalid addressing mode for JSR");
        std::exit(-1);
    }

    auto low = Fetch();
    mmu->Read(0x100 + (word)s);  // Run the third cycle
    mmu->Write(0x100 + (word)(s--), (byte)(pc >> 8));
    mmu->Write(0x100 + (word)(s--), (byte)pc);

    pc = ((word)Fetch()) << 8 | (word)low;
}

void Cpu::LDA(Opcode opcode) {
    switch (opcode.mode) {
        case AddressingMode::Immediate: {
            a = Fetch();
            break;
        }
        case AddressingMode::ZeroPage: {
            auto address = (word)Fetch();
            a = mmu->Read(address);
            break;
        }
        case AddressingMode::ZeroPageX: {
            auto address = ZeroPageIndexed(Fetch(), x);
            a = mmu->Read(address);
            break;
        }
        case AddressingMode::Absolute: {
            auto address = Absolute();
            a = mmu->Read(address);
            break;
        }
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed: {
            auto address = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            a = mmu->Read(address);
            break;
        }
        case AddressingMode::IndirectX: {
            auto address = IndirectX();
            a = mmu->Read(address);
            break;
        }
        case AddressingMode::IndirectY: {
            auto address = IndirectY();
            a = mmu->Read(address);
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for LDA");
            std::exit(-1);
    }

    p.flags.Zero = a == 0;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::NOP(Opcode opcode) {
    switch (opcode.mode) {
        case AddressingMode::Implied:
            mmu->Read(pc);  // Dummy Read
            break;
        default:
            spdlog::error("Invalid addressing mode for NOP");
            std::exit(-1);
    }
}

void Cpu::SEC(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for SEC");
        std::exit(-1);
    }

    p.flags.Carry = true;
    mmu->Read(pc);  // Dummy Read
}

void Cpu::SED(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for SED");
        std::exit(-1);
    }

    p.flags.Decimal = true;
    mmu->Read(pc);  // Dummy Read
}

void Cpu::SEI(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for SEI");
        std::exit(-1);
    }

    p.flags.InterruptDisable = true;
    mmu->Read(pc);  // Dummy Read
}

void Cpu::BCS(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BCS");
        std::exit(-1);
    }

    PerformRelativeBranch(p.flags.Carry);
}

void Cpu::BCC(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BCC");
        std::exit(-1);
    }

    PerformRelativeBranch(!p.flags.Carry);
}

void Cpu::BEQ(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BEQ");
        std::exit(-1);
    }

    PerformRelativeBranch(p.flags.Zero);
}

void Cpu::BNE(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BNE");
        std::exit(-1);
    }

    PerformRelativeBranch(!p.flags.Zero);
}

void Cpu::CLC(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for CLC");
        std::exit(-1);
    }

    p.flags.Carry = false;
    mmu->Read(pc);  // Dummy Read
}

void Cpu::STA(Opcode opcode) {
    word address;
    switch (opcode.mode) {
        case AddressingMode::ZeroPage: {
            address = (word)Fetch();
            break;
        }
        case AddressingMode::ZeroPageX:
            address = ZeroPageIndexed(Fetch(), x);
            break;
        case AddressingMode::Absolute:
            address = Absolute();
            break;
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed:
            address = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            break;

        case AddressingMode::IndirectX:
            address = IndirectX();
            break;
        case AddressingMode::IndirectY:
            address = IndirectY();
            break;
        default:
            spdlog::error("Invalid addressing mode for STA");
            std::exit(-1);
    }

    mmu->Write(address, a);
}

void Cpu::PerformRelativeBranch(bool condition) {
    auto operand = Fetch();
    if (!condition)
        return;

    mmu->Read(pc);  // Dummy Read
    cyc += 1;       // Branch taken cycle
    auto [pch, pcl] = splitWord(pc);
    auto incorrect = joinWord(pch, pcl + operand);
    auto correct = joinWord(pch, pcl) + operand;

    pc = incorrect;

    if (incorrect != correct) {  // There was a page crossing
        mmu->Read(incorrect);    // Dummy read
        pc = correct;
        cyc += 1;  // Oops cycle
    }
}
