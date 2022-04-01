//
// Created by rudolph on 20/3/22.
//

#include <array>

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

    // TODO: Determine this by reading the reset vector. For nestest
    //       hardcoded to 0xC000
    pc = 0xC000;
}

void Cpu::Tick() {
    auto opcode_byte = Fetch();
    auto opcode = DecodeOpcode(opcode_byte);
    return ExecuteOpcode(opcode);
}

void Cpu::ExecuteOpcode(Opcode opcode) {
    switch (opcode.class_) {
        OPCODE_CASE(JMP)
        OPCODE_CASE(LDX)
        OPCODE_CASE(STX)
        OPCODE_CASE(JSR)
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

std::pair<word, word> Cpu::AbsoluteIndexed(byte offset) {
    auto low = Fetch();
    auto high = Fetch();
    word supplied = (word)high << 8 | (word)(low + offset);
    word corrected = ((word)high << 8 | (word)low) + offset;
    return {supplied, corrected};
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
            auto [supplied, corrected] = AbsoluteIndexed(y);
            x = mmu->Read(supplied);

            if (supplied != corrected) {  // There was a page crossing
                x = mmu->Read(corrected);
            }

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

    pc = (word)Fetch() << 8 | (word)low;
}
