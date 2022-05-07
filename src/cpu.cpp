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

// Starting address for jump vectors
enum class JumpVectors : word {
    NMI = 0xFFFA,
    RES = 0xFFFC,
    IRQ = 0xFFFE,
};

Cpu::Cpu(std::shared_ptr<Mmu> mmu_) : mmu(std::move(mmu_)) {
    // Set Power-Up State. Reference:
    // https://www.nesdev.org/wiki/CPU_power_up_state
    a = x = y = 0x00;
    p.value = 0x34;  // Differs from nesdev to match with nestest.nes
    s = 0xFD;
}

void Cpu::Start() {
    // The whole RESET procedure when the CPU is turned on takes 7 cycles
    mmu->CpuRead(0x0000);      // The address does not matter. Hold during reset
    mmu->CpuRead(0x0001);      // First Start State
    mmu->CpuRead(0x0100 + s);  // Second Start State
    mmu->CpuRead(0x0100 + s - 1);  // Third Start State
    mmu->CpuRead(0x0100 + s - 2);  // Fourth Start State
    auto pcl = mmu->CpuRead(static_cast<word>(JumpVectors::RES));
    auto pch = mmu->CpuRead(static_cast<word>(JumpVectors::RES) + 1);
    pc = (word)pch << 8 | (word)pcl;
    // The eight cycle is the first opcode fetch
}

void Cpu::Reset() {
    // TODO: Use the reset procedure for a hard reset
}

void Cpu::Tick() {
    auto opcode_byte = Fetch();
    auto opcode = DecodeOpcode(opcode_byte);

    // PC and cpu cycle values need to be from before the opcode fetch
    spdlog::debug(
        "{:04X}  {:02X} A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} CYC:{}",
        pc - 1, opcode_byte, a, x, y, p.value, s, mmu->CpuCycles() - 1);

    ExecuteOpcode(opcode);
}

void Cpu::ExecuteOpcode(Opcode opcode) {
    switch (opcode.class_) {
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
        OPCODE_CASE(BVC)
        OPCODE_CASE(BVS)
        OPCODE_CASE(CLC)
        OPCODE_CASE(CLD)
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
        case OpcodeClass::JAM:
            spdlog::error("Unimplemented or Unknown opcode {:#4X} at {:#6X}",
                          opcode.opcode, pc - 1);
            std::exit(-1);
    }
}

byte Cpu::Fetch() {
    auto value = mmu->CpuRead(pc++);
    return value;
}

// Addressing Modes
word Cpu::Indirect() {
    auto pointer = Absolute();
    auto [high, low] = splitWord(pointer);

    auto pcl = (word)mmu->CpuRead(joinWord(high, low));
    // PCH is fetched from the same page as PCL i.e. only increment the low byte
    auto pch = (word)mmu->CpuRead(joinWord(high, low + 1));

    return (pch << 8) | pcl;
}

word Cpu::ZeroPageIndexed(byte offset) {
    auto operand = Fetch();
    mmu->CpuRead((word)operand);
    return (word)(operand + offset) & 0xFF;
}

word Cpu::Absolute() {
    word low = (word)Fetch();   // Fetch low byte
    word high = (word)Fetch();  // Fetch high byte

    return (high << 8) | low;
}

std::pair<word, bool> Cpu::AbsoluteIndexed(byte offset) {
    auto low = Fetch();
    auto high = Fetch();
    word supplied = joinWord(high, low + offset);
    word corrected = joinWord(high, low) + offset;

    mmu->CpuRead(supplied);
    auto effective = supplied;
    bool page_crossed = false;

    if (supplied != corrected) {
        effective = corrected;
        page_crossed = true;
    }

    return {effective, page_crossed};
}

word Cpu::IndirectX() {
    auto pointer = Fetch();

    mmu->CpuRead((word)pointer);  // Dummy read
    pointer += x;

    auto low = (word)mmu->CpuRead(pointer & 0xFF);
    auto high = (word)mmu->CpuRead((pointer + 1) & 0xFF);

    return (high << 8) | low;
}

word Cpu::IndirectY() {
    auto pointer = Fetch();

    auto low = mmu->CpuRead(pointer & 0xFF);
    auto high = mmu->CpuRead((pointer + 1) & 0xFF);

    word supplied = joinWord(high, low + y);
    word corrected = joinWord(high, low) + (word)y;

    mmu->CpuRead(supplied);
    auto effective = supplied;

    if (supplied != corrected) {
        effective = corrected;
        mmu->IncCpuCycles();
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
    x = FetchOperandForReadOpcodes(opcode, "LDX");
    p.flags.Zero = (x == 0x00);
    p.flags.Negative = isBitSet(x, 7);
}

void Cpu::LDY(Opcode opcode) {
    y = FetchOperandForReadOpcodes(opcode, "LDY");
    p.flags.Zero = (y == 0x00);
    p.flags.Negative = isBitSet(y, 7);
}

void Cpu::STX(Opcode opcode) {
    word address;
    switch (opcode.mode) {
        case AddressingMode::Absolute:
            address = Absolute();
            break;
        case AddressingMode::ZeroPage:
            address = (word)Fetch();
            break;
        case AddressingMode::ZeroPageY:
            address = ZeroPageIndexed(y);
            break;
        default:
            spdlog::error("Invalid addressing mode for STX");
            std::exit(-1);
    }

    mmu->CpuWrite(address, x);
}

void Cpu::STY(Opcode opcode) {
    word address;
    switch (opcode.mode) {
        case AddressingMode::Absolute:
            address = Absolute();
            break;
        case AddressingMode::ZeroPage:
            address = (word)Fetch();
            break;
        case AddressingMode::ZeroPageX:
            address = ZeroPageIndexed(x);
            break;
        default:
            spdlog::error("Invalid addressing mode for STY");
            std::exit(-1);
    }

    mmu->CpuWrite(address, y);
}

void Cpu::JSR(Opcode opcode) {
    if (opcode.mode != AddressingMode::Absolute) {
        spdlog::error("Invalid addressing mode for JSR");
        std::exit(-1);
    }

    auto low = Fetch();
    mmu->CpuRead(0x100 + s);  // Run the third cycle
    mmu->CpuWrite(0x100 + s--, (byte)(pc >> 8));
    mmu->CpuWrite(0x100 + s--, (byte)pc);

    pc = ((word)Fetch()) << 8 | (word)low;
}

void Cpu::RTS(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for RTS");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read

    mmu->CpuRead(0x100 + s++);
    auto pcl = mmu->CpuRead(0x100 + s++);
    auto pch = mmu->CpuRead(0x100 + s);

    pc = (word)pch << 8 | (word)pcl;
    mmu->CpuRead(pc++);
}

void Cpu::RTI(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for RTI");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    mmu->CpuRead(0x100 + s++);

    auto bit5 = p.value & 0x20;
    p.value = mmu->CpuRead(0x100 + s++);
    p.value |= bit5;  // Bit 5 is left as is

    pc = (word)mmu->CpuRead(0x100 + s++);
    pc |= (word)mmu->CpuRead(0x100 + s) << 8;
}

void Cpu::LDA(Opcode opcode) {
    a = FetchOperandForReadOpcodes(opcode, "LDA");
    p.flags.Zero = a == 0;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::NOP(Opcode opcode) {
    switch (opcode.mode) {
        case AddressingMode::Implied:
            mmu->CpuRead(pc);  // Dummy Read
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
    mmu->CpuRead(pc);  // Dummy Read
}

void Cpu::SED(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for SED");
        std::exit(-1);
    }

    p.flags.Decimal = true;
    mmu->CpuRead(pc);  // Dummy Read
}

void Cpu::SEI(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for SEI");
        std::exit(-1);
    }

    p.flags.InterruptDisable = true;
    mmu->CpuRead(pc);  // Dummy Read
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
    mmu->CpuRead(pc);  // Dummy Read
}

void Cpu::CLD(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for CLD");
        std::exit(-1);
    }

    p.flags.Decimal = false;
    mmu->CpuRead(pc);  // Dummy Read
}

void Cpu::CLV(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for CLV");
        std::exit(-1);
    }

    p.flags.Overflow = false;
    mmu->CpuRead(pc);  // Dummy Read
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
        case AddressingMode::AbsoluteYIndexed: {
            auto [effective, crossed] = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            address = effective;
            break;
        }
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

    mmu->CpuWrite(address, a);
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

    auto operand = mmu->CpuRead(address);
    p.flags.Negative = isBitSet(operand, 7);
    p.flags.Overflow = isBitSet(operand, 6);
    p.flags.Zero = (a & operand) == 0;
}

void Cpu::PHP(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PHP");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read

    auto status_register = p;
    // Bit 5 is always set to 1 when P is pushed, and bit 4 when pushed by
    // PHP/BRK
    status_register.flags.B = 0b11;
    mmu->CpuWrite(0x100 + s--, status_register.value);
}

void Cpu::PHA(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PHA");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    mmu->CpuWrite(0x100 + s--, a);
}

void Cpu::PLA(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PLA");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read

    mmu->CpuRead(0x100 + s++);
    a = mmu->CpuRead(0x100 + s);
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::PLP(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for PLP");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read

    mmu->CpuRead(0x100 + s++);

    // The B flag is not affected by the PULL. It should remain same
    auto oldBValue = p.flags.B;
    p.value = mmu->CpuRead(0x100 + s);
    p.flags.B = oldBValue;
}

void Cpu::CMP(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "CMP");

    auto result = a - operand;
    p.flags.Zero = result == 0x00;
    p.flags.Negative = isBitSet(result, 7);
    // The carry flag is set if no borrow or greater than or equal for A - M
    p.flags.Carry = a >= operand;
}

void Cpu::CPY(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "CPY");

    auto result = y - operand;
    p.flags.Zero = result == 0x00;
    p.flags.Negative = isBitSet(result, 7);
    p.flags.Carry = y >= operand;
}

void Cpu::CPX(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "CPX");

    auto result = x - operand;
    p.flags.Zero = result == 0x00;
    p.flags.Negative = isBitSet(result, 7);
    p.flags.Carry = x >= operand;
}

void Cpu::AND(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "AND");

    a &= operand;
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::ORA(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "ORA");

    a |= operand;
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::EOR(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "EOR");

    a ^= operand;
    p.flags.Zero = a == 0x00;
    p.flags.Negative = isBitSet(a, 7);
}

void Cpu::ADC(Opcode opcode) {
    byte operand = FetchOperandForReadOpcodes(opcode, "ADC");

    auto carry = p.flags.Carry;
    word result = (word)a + (word)operand + (word)carry;
    p.flags.Zero = (result & 0xFF) == 0x00;
    p.flags.Negative = isBitSet(result, 7);
    p.flags.Carry = result > 0xFF;
    // Reference:
    // https://github.com/OneLoneCoder/olcNES/blob/663e3777191c011135dfb6d40c887ae126970dd7/Part%20%233%20-%20Buses%2C%20Rams%2C%20Roms%20%26%20Mappers/olc6502.cpp#L589
    // http://www.6502.org/tutorials/vflag.html
    p.flags.Overflow =
        isBitSet((~((word)a ^ (word)operand) & ((word)a ^ result)), 7);

    a = (byte)result;
}

void Cpu::SBC(Opcode opcode) {
    auto operand = (word)FetchOperandForReadOpcodes(opcode, "SBC") ^ 0xFF;

    auto carry = p.flags.Carry;
    word result = (word)a + (word)operand + (word)carry;
    p.flags.Zero = (result & 0xFF) == 0x00;
    p.flags.Negative = isBitSet(result, 7);
    p.flags.Carry = result > 0xFF;
    // Reference:
    // https://github.com/OneLoneCoder/olcNES/blob/663e3777191c011135dfb6d40c887ae126970dd7/Part%20%233%20-%20Buses%2C%20Rams%2C%20Roms%20%26%20Mappers/olc6502.cpp#L589
    // http://www.6502.org/tutorials/vflag.html
    p.flags.Overflow = isBitSet((result ^ (word)a) & (result ^ operand), 7);

    a = (byte)result;
}

void Cpu::INX(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for INX");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoInc(x);
}

void Cpu::INY(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for INY");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoInc(y);
}

void Cpu::INC(Opcode opcode) {
    word address;
    switch (opcode.mode) {
        case AddressingMode::ZeroPage:
            address = (word)Fetch();
            break;
        case AddressingMode::ZeroPageX:
            address = ZeroPageIndexed(x);
            break;
        case AddressingMode::Absolute:
            address = Absolute();
            break;
        case AddressingMode::AbsoluteXIndexed: {
            auto [effective, crossed] = AbsoluteIndexed(x);
            address = effective;
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for INC");
            std::exit(-1);
    }

    auto operand = mmu->CpuRead(address);
    DoInc(operand);
    mmu->CpuWrite(address, operand);
}

void Cpu::DEC(Opcode opcode) {
    word address;
    switch (opcode.mode) {
        case AddressingMode::ZeroPage:
            address = (word)Fetch();
            break;
        case AddressingMode::ZeroPageX:
            address = ZeroPageIndexed(x);
            break;
        case AddressingMode::Absolute:
            address = Absolute();
            break;
        case AddressingMode::AbsoluteXIndexed: {
            auto [effective, crossed] = AbsoluteIndexed(x);
            address = effective;
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for DEC");
            std::exit(-1);
    }

    auto operand = mmu->CpuRead(address);
    DoDec(operand);
    mmu->CpuWrite(address, operand);
}

void Cpu::DEX(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for DEX");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoDec(x);
}

void Cpu::DEY(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for DEY");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoDec(y);
}

void Cpu::TAX(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for TAX");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoTransfer(x, a);
}

void Cpu::TAY(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for TAY");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoTransfer(y, a);
}

void Cpu::TXA(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for TXA");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoTransfer(a, x);
}

void Cpu::TSX(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for TSX");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoTransfer(x, s);
}

void Cpu::TXS(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for TXS");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    s = x;             // This one does not set the flags
}

void Cpu::TYA(Opcode opcode) {
    if (opcode.mode != AddressingMode::Implied) {
        spdlog::error("Invalid addressing mode for TYA");
        std::exit(-1);
    }

    mmu->CpuRead(pc);  // Dummy Read
    DoTransfer(a, y);
}

void Cpu::LSR(Opcode opcode) {
    PerformShiftOrRotate(opcode, "LSR");
}

void Cpu::ASL(Opcode opcode) {
    PerformShiftOrRotate(opcode, "ASL");
}

void Cpu::ROL(Opcode opcode) {
    PerformShiftOrRotate(opcode, "ROL");
}

void Cpu::ROR(Opcode opcode) {
    PerformShiftOrRotate(opcode, "ROR");
}

// Opcode Helpers
void Cpu::PerformRelativeBranch(bool condition) {
    auto operand = Fetch();
    if (!condition)
        return;

    mmu->CpuRead(pc);     // Dummy Read
    mmu->IncCpuCycles();  // Branch taken cycle
    auto [pch, pcl] = splitWord(pc);
    word incorrect = joinWord(pch, pcl + operand);
    word correct = joinWord(pch, pcl) + (word)operand;

    pc = incorrect;

    if (incorrect != correct) {   // There was a page crossing
        mmu->CpuRead(incorrect);  // Dummy read
        pc = correct;
        mmu->IncCpuCycles();  // Oops cycle
    }
}

byte Cpu::FetchOperandForReadOpcodes(const Opcode& opcode, const char* repr) {
    byte operand{};
    switch (opcode.mode) {
        case AddressingMode::Immediate:
            operand = Fetch();
            break;
        case AddressingMode::ZeroPage:
            operand = mmu->CpuRead((word)Fetch());
            break;
        case AddressingMode::ZeroPageX:
            operand = mmu->CpuRead(ZeroPageIndexed(x));
            break;
        case AddressingMode::ZeroPageY:
            operand = mmu->CpuRead(ZeroPageIndexed(y));
            break;
        case AddressingMode::Absolute:
            operand = mmu->CpuRead(Absolute());
            break;
        case AddressingMode::AbsoluteXIndexed:
        case AddressingMode::AbsoluteYIndexed: {
            auto [address, crossed] = AbsoluteIndexed(
                opcode.mode == AddressingMode::AbsoluteXIndexed ? x : y);
            operand = mmu->CpuRead(address);
            if (crossed)
                mmu->IncCpuCycles();
            break;
        }
        case AddressingMode::IndirectX:
            operand = mmu->CpuRead(IndirectX());
            break;
        case AddressingMode::IndirectY:
            operand = mmu->CpuRead(IndirectY());
            break;
        default:
            spdlog::error("Invalid addressing mode for {}", repr);
            std::exit(-1);
    }

    return operand;
}

void Cpu::DoInc(byte& reg) {
    reg = reg + 1;
    p.flags.Zero = reg == 0x00;
    p.flags.Negative = isBitSet(reg, 7);
}

void Cpu::DoDec(byte& reg) {
    reg = reg - 1;
    p.flags.Zero = reg == 0x00;
    p.flags.Negative = isBitSet(reg, 7);
}

void Cpu::DoTransfer(byte& dest, byte src) {
    dest = src;
    p.flags.Zero = dest == 0x00;
    p.flags.Negative = isBitSet(dest, 7);
}

void Cpu::PerformShiftOrRotate(const Opcode& opcode, const char* repr) {
    byte operand{};
    word effective{};
    switch (opcode.mode) {
        case AddressingMode::Accumulator:
            operand = a;
            break;
        case AddressingMode::ZeroPage:
            effective = (word)Fetch();
            operand = mmu->CpuRead(effective);
            break;
        case AddressingMode::ZeroPageX:
            effective = ZeroPageIndexed(x);
            operand = mmu->CpuRead(effective);
            break;
        case AddressingMode::Absolute:
            effective = Absolute();
            operand = mmu->CpuRead(effective);
            break;
        case AddressingMode::AbsoluteXIndexed: {
            auto [address, crossed] = AbsoluteIndexed(x);
            effective = address;
            operand = mmu->CpuRead(effective);
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for ASL");
            std::exit(-1);
    }

    if (opcode.mode != AddressingMode::Accumulator) {
        mmu->CpuWrite(effective, operand);  // Dummy write
    }

    switch (opcode.class_) {
        case OpcodeClass::ASL:
            p.flags.Carry = isBitSet(operand, 7);
            operand <<= 1;
            break;
        case OpcodeClass::LSR:
            p.flags.Carry = isBitSet(operand, 0);
            operand >>= 1;
            break;
        case OpcodeClass::ROL: {
            auto carry = isBitSet(operand, 7);
            operand <<= 1;
            operand |= p.flags.Carry ? 0x01 : 0x00;
            p.flags.Carry = carry;
            break;
        }
        case OpcodeClass::ROR: {
            auto carry = isBitSet(operand, 0);
            operand >>= 1;
            operand |= p.flags.Carry ? 0x80 : 0x00;
            p.flags.Carry = carry;
            break;
        }
        default:
            spdlog::error("Invalid addressing mode for {}", repr);
            std::exit(-1);
    }

    p.flags.Negative = isBitSet(operand, 7);
    p.flags.Zero = operand == 0x00;

    // Write back the value
    switch (opcode.mode) {
        case AddressingMode::Accumulator:
            a = operand;
            break;
        default:
            mmu->CpuWrite(effective, operand);
            break;
    }
}
