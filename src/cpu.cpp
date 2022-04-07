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
        OPCODE_CASE(AND)
        OPCODE_CASE(BCC)
        OPCODE_CASE(BCS)
        OPCODE_CASE(BEQ)
        OPCODE_CASE(BIT)
        OPCODE_CASE(BMI)
        OPCODE_CASE(BNE)
        OPCODE_CASE(BPL)
        OPCODE_CASE(BVC)
        OPCODE_CASE(BVS)
        OPCODE_CASE(CLC)
        OPCODE_CASE(CLD)
        OPCODE_CASE(CLV)
        OPCODE_CASE(CMP)
        OPCODE_CASE(EOR)
        OPCODE_CASE(JMP)
        OPCODE_CASE(JSR)
        OPCODE_CASE(LDA)
        OPCODE_CASE(LDX)
        OPCODE_CASE(NOP)
        OPCODE_CASE(ORA)
        OPCODE_CASE(PHA)
        OPCODE_CASE(PHP)
        OPCODE_CASE(PLA)
        OPCODE_CASE(PLP)
        OPCODE_CASE(RTS)
        OPCODE_CASE(SEC)
        OPCODE_CASE(SED)
        OPCODE_CASE(SEI)
        OPCODE_CASE(STA)
        OPCODE_CASE(STX)
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
word Cpu::Indirect() {
    auto pointer = Absolute();
    auto latch = mmu->Read(pointer);

    word result = (word)(mmu->Read(pointer + 1)) << 8;
    result |= latch;

    return result;
}

word Cpu::ZeroPageIndexed(byte offset) {
    auto operand = Fetch();
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
    switch (opcode.mode) {
        case AddressingMode::Absolute:
            pc = Absolute();
            break;
        case AddressingMode::Indirect: {
            pc = Indirect();
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
            auto address = ZeroPageIndexed(y);
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
                address = ZeroPageIndexed(y);
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
    mmu->Read(0x100 + s);  // Run the third cycle
    mmu->Write(0x100 + s--, (byte)(pc >> 8));
    mmu->Write(0x100 + s--, (byte)pc);

    pc = ((word)Fetch()) << 8 | (word)low;
}

void Cpu::RTS(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for RTS");
        std::exit(-1);
    }

    mmu->Read(pc);  // Dummy Read

    mmu->Read(0x100 + s++);
    auto pcl = mmu->Read(0x100 + s++);
    auto pch = mmu->Read(0x100 + s);

    pc = (word)pch << 8 | (word)pcl;
    mmu->Read(pc++);
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
            auto address = ZeroPageIndexed(x);
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

void Cpu::BVS(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BVS");
        std::exit(-1);
    }

    PerformRelativeBranch(p.flags.Overflow);
}

void Cpu::BVC(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BVC");
        std::exit(-1);
    }

    PerformRelativeBranch(!p.flags.Overflow);
}

void Cpu::BPL(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BPL");
        std::exit(-1);
    }

    PerformRelativeBranch(!p.flags.Negative);
}

void Cpu::BMI(Opcode opcode) {
    if (opcode.mode != AddressingMode::Relative) {
        spdlog::error("Invalid addressing mode for BMI");
        std::exit(-1);
    }

    PerformRelativeBranch(p.flags.Negative);
}

void Cpu::CLC(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for CLC");
        std::exit(-1);
    }

    p.flags.Carry = false;
    mmu->Read(pc);  // Dummy Read
}

void Cpu::CLD(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for CLD");
        std::exit(-1);
    }

    p.flags.Decimal = false;
    mmu->Read(pc);  // Dummy Read
}

void Cpu::CLV(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for CLV");
        std::exit(-1);
    }

    p.flags.Overflow = false;
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
            address = ZeroPageIndexed(x);
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

void Cpu::BIT(Opcode opcode) {
    word address;
    switch (opcode.mode) {
        case AddressingMode::ZeroPage:
            address = (word)Fetch();
            break;
        case AddressingMode::Absolute:
            address = Absolute();
            break;
        default:
            spdlog::error("Invalid addressing mode for BIT");
            std::exit(-1);
    }

    auto operand = mmu->Read(address);
    p.flags.Negative = isBitSet(operand, 7);
    p.flags.Overflow = isBitSet(operand, 6);
    p.flags.Zero = (a & operand) == 0;
}

void Cpu::PHP(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PHP");
        std::exit(-1);
    }

    mmu->Read(pc);  // Dummy Read

    auto status_register = p;
    // Bit 5 is always set to 1 when P is pushed, and bit 4 when pushed by
    // PHP/BRK
    status_register.flags.B = 0b11;
    mmu->Write(0x100 + s--, status_register.value);
}

void Cpu::PHA(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PHA");
        std::exit(-1);
    }

    mmu->Read(pc);  // Dummy Read
    mmu->Write(0x100 + s--, a);
}

void Cpu::PLA(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PLA");
        std::exit(-1);
    }

    mmu->Read(pc);  // Dummy Read

    mmu->Read(0x100 + s++);
    a = mmu->Read(0x100 + s);
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::PLP(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PLP");
        std::exit(-1);
    }

    mmu->Read(pc);  // Dummy Read

    mmu->Read(0x100 + s++);

    // The B flag is not affected by the PULL. It should remain same
    auto oldBValue = p.flags.B;
    p.value = mmu->Read(0x100 + s);
    p.flags.B = oldBValue;
}

void Cpu::CMP(Opcode opcode) {
    byte operand{};
    switch (opcode.mode) {
        case AddressingMode::Immediate:
            operand = Fetch();
            break;
        case AddressingMode::ZeroPage:
            operand = mmu->Read((word)Fetch());
            break;
        case AddressingMode::ZeroPageX:
            operand = mmu->Read(ZeroPageIndexed(x));
            break;
        case AddressingMode::Absolute:
            operand = mmu->Read(Absolute());
            break;
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed: {
            auto address = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            operand = mmu->Read(address);
            break;
        }
        case AddressingMode::IndirectX:
            operand = mmu->Read(IndirectX());
            break;
        case AddressingMode::IndirectY:
            operand = mmu->Read(IndirectY());
            break;
        default:
            spdlog::error("Invalid addressing mode for CMP");
            std::exit(-1);
    }

    auto result = a - operand;
    p.flags.Zero = result == 0x00;
    p.flags.Negative = isBitSet(result, 7);
    // The carry flag is set if no borrow or greater than or equal for A - M
    p.flags.Carry = a >= operand;
}

void Cpu::AND(Opcode opcode) {
    byte operand{};
    switch (opcode.mode) {
        case AddressingMode::Immediate:
            operand = Fetch();
            break;
        case AddressingMode::ZeroPage:
            operand = mmu->Read((word)Fetch());
            break;
        case AddressingMode::ZeroPageX:
            operand = mmu->Read(ZeroPageIndexed(x));
            break;
        case AddressingMode::Absolute:
            operand = mmu->Read(Absolute());
            break;
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed: {
            auto address = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            operand = mmu->Read(address);
            break;
        }
        case AddressingMode::IndirectX:
            operand = mmu->Read(IndirectX());
            break;
        case AddressingMode::IndirectY:
            operand = mmu->Read(IndirectY());
            break;
        default:
            spdlog::error("Invalid addressing mode for AND");
            std::exit(-1);
    }

    a &= operand;
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::ORA(Opcode opcode) {
    byte operand{};
    switch (opcode.mode) {
        case AddressingMode::Immediate:
            operand = Fetch();
            break;
        case AddressingMode::ZeroPage:
            operand = mmu->Read((word)Fetch());
            break;
        case AddressingMode::ZeroPageX:
            operand = mmu->Read(ZeroPageIndexed(x));
            break;
        case AddressingMode::Absolute:
            operand = mmu->Read(Absolute());
            break;
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed: {
            auto address = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            operand = mmu->Read(address);
            break;
        }
        case AddressingMode::IndirectX:
            operand = mmu->Read(IndirectX());
            break;
        case AddressingMode::IndirectY:
            operand = mmu->Read(IndirectY());
            break;
        default:
            spdlog::error("Invalid addressing mode for ORA");
            std::exit(-1);
    }

    a |= operand;
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::EOR(Opcode opcode) {
    byte operand{};
    switch (opcode.mode) {
        case AddressingMode::Immediate:
            operand = Fetch();
            break;
        case AddressingMode::ZeroPage:
            operand = mmu->Read((word)Fetch());
            break;
        case AddressingMode::ZeroPageX:
            operand = mmu->Read(ZeroPageIndexed(x));
            break;
        case AddressingMode::Absolute:
            operand = mmu->Read(Absolute());
            break;
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed: {
            auto address = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            operand = mmu->Read(address);
            break;
        }
        case AddressingMode::IndirectX:
            operand = mmu->Read(IndirectX());
            break;
        case AddressingMode::IndirectY:
            operand = mmu->Read(IndirectY());
            break;
        default:
            spdlog::error("Invalid addressing mode for EOR");
            std::exit(-1);
    }

    a ^= operand;
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

// Opcode Helpers
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
