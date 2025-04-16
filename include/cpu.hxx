#pragma once

#include <spdlog/spdlog.h>

#include <array>
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/base.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <tuple>

#include "constants.hxx"

#define OPCODE_CASE(opc) \
    case OpcodeClass::opc: \
        opc(opcode); \
        break;

// Effective address and if there was a page crossing
using EffectiveAddress = std::tuple<word, bool>;

template<typename T>
concept SystemBus = requires(T bus, word address, byte data) {
    bus.tick();

    { bus.ticked_cpu_read(address) } -> std::convertible_to<byte>;
    { bus.cpu_read(address) } -> std::convertible_to<byte>;

    bus.ticked_cpu_write(address, data);
    bus.cpu_write(address, data);
};

enum class OpcodeClass {
    // Add Memory to Accumulator With Carry
    ADC,
    // AND Memory with Accumulator
    AND,
    // Shift Left One Bit (Accumulator or Memory)
    ASL,
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
    // Force Break
    BRK,
    // Branch on Overflow Clear
    BVC,
    // Branch on Overflow Set
    BVS,
    // Clear Carry Flag
    CLC,
    // Clear Decimal Mode
    CLD,
    // Clear Interrupt Disable Bit
    CLI,
    // Clear Overflow flag
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
    // Shift One Bit Right (Memory or Accumulator)
    LSR,
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
    // Rotate One Bit Left (Memory or Accumulator)
    ROL,
    // Rotate One Bit Right (Memory or Accumulator)
    ROR,
    // Return from Interrupt
    RTI,
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
    // Store Y to Memory
    STY,
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
    OpcodeClass opcode_class;
    byte opcode;
    AddressingMode addressing_mode;

    size_t length;
    unsigned int cycles;

    const char* label;
};

struct ExecutedOpcode {
    uint64_t start_cycle{};
    word pc{};
    byte opcode{};
    byte arg1{};
    byte arg2{};
};

enum class StatusFlag : byte {
    Carry = (1U << 0U), // C
    Zero = (1U << 1U), // Z
    InterruptDisable = (1U << 2U), // I
    Decimal = (1U << 3U), // D
    B = (0b11U << 4U), // No CPU effect, bits 45
    Overflow = (1U << 6U), // V
    Negative = (1U << 7U), // N
};

template<SystemBus BusType>
class Cpu {
  private:
    byte a{0x00}; // Accumulator
    byte x{0x00}, y{0x00}; // General purpose registers
    word pc{0x0000}; // Program counter
    byte s{0xFD}; // Stack pointer
    byte p{0x34}; // Status register

    std::shared_ptr<BusType> bus{};
    InterruptRequestFlag nmi_requested, irq_requested;

    boost::circular_buffer<ExecutedOpcode> executed_opcodes{30};

    // Addressing Modes

    // Takes 2 cycles
    EffectiveAddress absolute_addressing();
    // Takes 4 cycles
    EffectiveAddress indirect_addressing();
    // Takes 1 cycle
    EffectiveAddress zero_page_addressing();
    // Takes 2 cycles
    EffectiveAddress zero_page_x_addressing();
    // Takes 2 cycles
    EffectiveAddress zero_page_y_addressing();
    // Takes 2 cycles; 3 if page crossed
    EffectiveAddress absolute_x_indexed_addressing();
    // Takes 2 cycles; 3 if page crossed
    EffectiveAddress absolute_y_indexed_addressing();
    // Takes 4 cycles
    EffectiveAddress indirect_x_addressing();
    // Takes 4 cycles; 5 if page crossed
    EffectiveAddress indirect_y_addressing();

    EffectiveAddress fetch_effective_address(AddressingMode mode);

    // Opcodes
    void ADC(const Opcode& opcode);
    void AND(const Opcode& opcode);
    void ASL(const Opcode& opcode);
    void BCC(const Opcode& opcode);
    void BCS(const Opcode& opcode);
    void BEQ(const Opcode& opcode);
    void BIT(const Opcode& opcode);
    void BMI(const Opcode& opcode);
    void BNE(const Opcode& opcode);
    void BPL(const Opcode& opcode);
    void BRK(const Opcode& opcode);
    void BVC(const Opcode& opcode);
    void BVS(const Opcode& opcode);
    void CLC(const Opcode& opcode);
    void CLD(const Opcode& opcode);
    void CLI(const Opcode& opcode);
    void CLV(const Opcode& opcode);
    void CMP(const Opcode& opcode);
    void CPX(const Opcode& opcode);
    void CPY(const Opcode& opcode);
    void DEC(const Opcode& opcode);
    void DEX(const Opcode& opcode);
    void DEY(const Opcode& opcode);
    void EOR(const Opcode& opcode);
    void INC(const Opcode& opcode);
    void INX(const Opcode& opcode);
    void INY(const Opcode& opcode);
    void JAM(const Opcode& opcode);
    void JMP(const Opcode& opcode);
    void JSR(const Opcode& opcode);
    void LDA(const Opcode& opcode);
    void LDX(const Opcode& opcode);
    void LDY(const Opcode& opcode);
    void LSR(const Opcode& opcode);
    void NOP(const Opcode& opcode);
    void ORA(const Opcode& opcode);
    void PHA(const Opcode& opcode);
    void PHP(const Opcode& opcode);
    void PLA(const Opcode& opcode);
    void PLP(const Opcode& opcode);
    void ROL(const Opcode& opcode);
    void ROR(const Opcode& opcode);
    void RTI(const Opcode& opcode);
    void RTS(const Opcode& opcode);
    void SBC(const Opcode& opcode);
    void SEC(const Opcode& opcode);
    void SED(const Opcode& opcode);
    void SEI(const Opcode& opcode);
    void STA(const Opcode& opcode);
    void STX(const Opcode& opcode);
    void STY(const Opcode& opcode);
    void TAX(const Opcode& opcode);
    void TAY(const Opcode& opcode);
    void TSX(const Opcode& opcode);
    void TXA(const Opcode& opcode);
    void TXS(const Opcode& opcode);
    void TYA(const Opcode& opcode);

    // Opcode helpers
    void relative_branch_on(bool condition);
    void cmp_reg_and_mem(const Opcode& opcode, byte reg);

    void check_interrupts();

  public:
    static constexpr word NMI_VECTOR = 0xFFFA;
    static constexpr word RESET_VECTOR = 0xFFFC;
    static constexpr word IRQ_VECTOR = 0xFFFE;

    Cpu() = default;

    Cpu(std::shared_ptr<BusType> bus,
        InterruptRequestFlag nmi_requested,
        InterruptRequestFlag irq_requested) :
        bus{std::move(bus)},
        nmi_requested{std::move(nmi_requested)},
        irq_requested{std::move(irq_requested)} {}

    [[nodiscard]] bool flag_set(StatusFlag flag) const {
        return (p & static_cast<byte>(flag)) != 0;
    }

    void update_flag(StatusFlag flag, const bool value) {
        if (value) {
            p |= static_cast<byte>(flag);
        } else {
            p &= ~static_cast<byte>(flag);
        }
    }

    // Runs the CPU startup procedure. Should run for 7 NES cycles
    void start();

    void reset() {
        spdlog::error("CPU reset procedure not implemented");
    };

    byte fetch() {
        return bus->ticked_cpu_read(pc++);
    }

    void step();
    void execute_opcode(Opcode opcode);

    // For setting random register state during opcode tests
    friend class Debugger;
};

inline word non_page_crossing_add(const word value, const word increment) {
    return (value & 0xFF00U) | ((value + increment) & 0xFFU);
}

static constexpr std::array<Opcode, 256> OPCODES{
    {
        {OpcodeClass::BRK, 0x00, AddressingMode::Implied, 1, 7, "BRK"},
        {OpcodeClass::ORA, 0x01, AddressingMode::IndirectX, 2, 6, "ORA"},
        {OpcodeClass::JAM, 0x02, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x03, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x04, AddressingMode::ZeroPage, 2, 3, "NOP"},
        {OpcodeClass::ORA, 0x05, AddressingMode::ZeroPage, 2, 3, "ORA"},
        {OpcodeClass::ASL, 0x06, AddressingMode::ZeroPage, 2, 5, "ASL"},
        {OpcodeClass::JAM, 0x07, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::PHP, 0x08, AddressingMode::Implied, 1, 3, "PHP"},
        {OpcodeClass::ORA, 0x09, AddressingMode::Immediate, 2, 2, "ORA"},
        {OpcodeClass::ASL, 0x0A, AddressingMode::Accumulator, 1, 2, "ASL"},
        {OpcodeClass::JAM, 0x0B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x0C, AddressingMode::Absolute, 3, 4, "NOP"},
        {OpcodeClass::ORA, 0x0D, AddressingMode::Absolute, 3, 4, "ORA"},
        {OpcodeClass::ASL, 0x0E, AddressingMode::Absolute, 3, 6, "ASL"},
        {OpcodeClass::JAM, 0x0F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BPL, 0x10, AddressingMode::Relative, 2, 2, "BPL"},
        {OpcodeClass::ORA, 0x11, AddressingMode::IndirectY, 2, 5, "ORA"},
        {OpcodeClass::JAM, 0x12, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x13, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x14, AddressingMode::ZeroPageX, 2, 4, "NOP"},
        {OpcodeClass::ORA, 0x15, AddressingMode::ZeroPageX, 2, 4, "ORA"},
        {OpcodeClass::ASL, 0x16, AddressingMode::ZeroPageX, 2, 6, "ASL"},
        {OpcodeClass::JAM, 0x17, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CLC, 0x18, AddressingMode::Implied, 1, 2, "CLC"},
        {OpcodeClass::ORA, 0x19, AddressingMode::AbsoluteYIndexed, 3, 4, "ORA"},
        {OpcodeClass::NOP, 0x1A, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0x1B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x1C, AddressingMode::AbsoluteXIndexed, 3, 4, "NOP"},
        {OpcodeClass::ORA, 0x1D, AddressingMode::AbsoluteXIndexed, 3, 4, "ORA"},
        {OpcodeClass::ASL, 0x1E, AddressingMode::AbsoluteXIndexed, 3, 7, "ASL"},
        {OpcodeClass::JAM, 0x1F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JSR, 0x20, AddressingMode::Absolute, 3, 6, "JSR"},
        {OpcodeClass::AND, 0x21, AddressingMode::IndirectX, 2, 6, "AND"},
        {OpcodeClass::JAM, 0x22, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x23, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BIT, 0x24, AddressingMode::ZeroPage, 2, 3, "BIT"},
        {OpcodeClass::AND, 0x25, AddressingMode::ZeroPage, 2, 3, "AND"},
        {OpcodeClass::ROL, 0x26, AddressingMode::ZeroPage, 2, 5, "ROL"},
        {OpcodeClass::JAM, 0x27, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::PLP, 0x28, AddressingMode::Implied, 1, 4, "PLP"},
        {OpcodeClass::AND, 0x29, AddressingMode::Immediate, 2, 2, "AND"},
        {OpcodeClass::ROL, 0x2A, AddressingMode::Accumulator, 1, 2, "ROL"},
        {OpcodeClass::JAM, 0x2B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BIT, 0x2C, AddressingMode::Absolute, 3, 4, "BIT"},
        {OpcodeClass::AND, 0x2D, AddressingMode::Absolute, 3, 4, "AND"},
        {OpcodeClass::ROL, 0x2E, AddressingMode::Absolute, 3, 6, "ROL"},
        {OpcodeClass::JAM, 0x2F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BMI, 0x30, AddressingMode::Relative, 2, 2, "BMI"},
        {OpcodeClass::AND, 0x31, AddressingMode::IndirectY, 2, 5, "AND"},
        {OpcodeClass::JAM, 0x32, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x33, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x34, AddressingMode::ZeroPageX, 2, 4, "NOP"},
        {OpcodeClass::AND, 0x35, AddressingMode::ZeroPageX, 2, 4, "AND"},
        {OpcodeClass::ROL, 0x36, AddressingMode::ZeroPageX, 2, 6, "ROL"},
        {OpcodeClass::JAM, 0x37, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::SEC, 0x38, AddressingMode::Implied, 1, 2, "SEC"},
        {OpcodeClass::AND, 0x39, AddressingMode::AbsoluteYIndexed, 3, 4, "AND"},
        {OpcodeClass::NOP, 0x3A, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0x3B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x3C, AddressingMode::AbsoluteXIndexed, 3, 4, "NOP"},
        {OpcodeClass::AND, 0x3D, AddressingMode::AbsoluteXIndexed, 3, 4, "AND"},
        {OpcodeClass::ROL, 0x3E, AddressingMode::AbsoluteXIndexed, 3, 7, "ROL"},
        {OpcodeClass::JAM, 0x3F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::RTI, 0x40, AddressingMode::Implied, 1, 6, "RTI"},
        {OpcodeClass::EOR, 0x41, AddressingMode::IndirectX, 2, 6, "EOR"},
        {OpcodeClass::JAM, 0x42, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x43, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x44, AddressingMode::ZeroPage, 2, 3, "NOP"},
        {OpcodeClass::EOR, 0x45, AddressingMode::ZeroPage, 2, 3, "EOR"},
        {OpcodeClass::LSR, 0x46, AddressingMode::ZeroPage, 2, 5, "LSR"},
        {OpcodeClass::JAM, 0x47, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::PHA, 0x48, AddressingMode::Implied, 1, 3, "PHA"},
        {OpcodeClass::EOR, 0x49, AddressingMode::Immediate, 2, 2, "EOR"},
        {OpcodeClass::LSR, 0x4A, AddressingMode::Accumulator, 1, 2, "LSR"},
        {OpcodeClass::JAM, 0x4B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JMP, 0x4C, AddressingMode::Absolute, 3, 3, "JMP"},
        {OpcodeClass::EOR, 0x4D, AddressingMode::Absolute, 3, 4, "EOR"},
        {OpcodeClass::LSR, 0x4E, AddressingMode::Absolute, 3, 6, "LSR"},
        {OpcodeClass::JAM, 0x4F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BVC, 0x50, AddressingMode::Relative, 2, 2, "BVC"},
        {OpcodeClass::EOR, 0x51, AddressingMode::IndirectY, 2, 5, "EOR"},
        {OpcodeClass::JAM, 0x52, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x53, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x54, AddressingMode::ZeroPageX, 2, 4, "NOP"},
        {OpcodeClass::EOR, 0x55, AddressingMode::ZeroPageX, 2, 4, "EOR"},
        {OpcodeClass::LSR, 0x56, AddressingMode::ZeroPageX, 2, 6, "LSR"},
        {OpcodeClass::JAM, 0x57, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CLI, 0x58, AddressingMode::Implied, 1, 2, "CLI"},
        {OpcodeClass::EOR, 0x59, AddressingMode::AbsoluteYIndexed, 3, 4, "EOR"},
        {OpcodeClass::NOP, 0x5A, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0x5B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x5C, AddressingMode::AbsoluteXIndexed, 3, 4, "NOP"},
        {OpcodeClass::EOR, 0x5D, AddressingMode::AbsoluteXIndexed, 3, 4, "EOR"},
        {OpcodeClass::LSR, 0x5E, AddressingMode::AbsoluteXIndexed, 3, 7, "LSR"},
        {OpcodeClass::JAM, 0x5F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::RTS, 0x60, AddressingMode::Implied, 1, 6, "RTS"},
        {OpcodeClass::ADC, 0x61, AddressingMode::IndirectX, 2, 6, "ADC"},
        {OpcodeClass::JAM, 0x62, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x63, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x64, AddressingMode::ZeroPage, 2, 3, "NOP"},
        {OpcodeClass::ADC, 0x65, AddressingMode::ZeroPage, 2, 3, "ADC"},
        {OpcodeClass::ROR, 0x66, AddressingMode::ZeroPage, 2, 5, "ROR"},
        {OpcodeClass::JAM, 0x67, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::PLA, 0x68, AddressingMode::Implied, 1, 4, "PLA"},
        {OpcodeClass::ADC, 0x69, AddressingMode::Immediate, 2, 2, "ADC"},
        {OpcodeClass::ROR, 0x6A, AddressingMode::Accumulator, 1, 2, "ROR"},
        {OpcodeClass::JAM, 0x6B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JMP, 0x6C, AddressingMode::Indirect, 3, 5, "JMP"},
        {OpcodeClass::ADC, 0x6D, AddressingMode::Absolute, 3, 4, "ADC"},
        {OpcodeClass::ROR, 0x6E, AddressingMode::Absolute, 3, 6, "ROR"},
        {OpcodeClass::JAM, 0x6F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BVS, 0x70, AddressingMode::Relative, 2, 2, "BVS"},
        {OpcodeClass::ADC, 0x71, AddressingMode::IndirectY, 2, 5, "ADC"},
        {OpcodeClass::JAM, 0x72, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x73, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x74, AddressingMode::ZeroPageX, 2, 4, "NOP"},
        {OpcodeClass::ADC, 0x75, AddressingMode::ZeroPageX, 2, 4, "ADC"},
        {OpcodeClass::ROR, 0x76, AddressingMode::ZeroPageX, 2, 6, "ROR"},
        {OpcodeClass::JAM, 0x77, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::SEI, 0x78, AddressingMode::Implied, 1, 2, "SEI"},
        {OpcodeClass::ADC, 0x79, AddressingMode::AbsoluteYIndexed, 3, 4, "ADC"},
        {OpcodeClass::NOP, 0x7A, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0x7B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x7C, AddressingMode::AbsoluteXIndexed, 3, 4, "NOP"},
        {OpcodeClass::ADC, 0x7D, AddressingMode::AbsoluteXIndexed, 3, 4, "ADC"},
        {OpcodeClass::ROR, 0x7E, AddressingMode::AbsoluteXIndexed, 3, 7, "ROR"},
        {OpcodeClass::JAM, 0x7F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0x80, AddressingMode::Immediate, 2, 2, "NOP"},
        {OpcodeClass::STA, 0x81, AddressingMode::IndirectX, 2, 6, "STA"},
        {OpcodeClass::NOP, 0x82, AddressingMode::Immediate, 2, 2, "NOP"},
        {OpcodeClass::JAM, 0x83, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::STY, 0x84, AddressingMode::ZeroPage, 2, 3, "STY"},
        {OpcodeClass::STA, 0x85, AddressingMode::ZeroPage, 2, 3, "STA"},
        {OpcodeClass::STX, 0x86, AddressingMode::ZeroPage, 2, 3, "STX"},
        {OpcodeClass::JAM, 0x87, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::DEY, 0x88, AddressingMode::Implied, 1, 2, "DEY"},
        {OpcodeClass::NOP, 0x89, AddressingMode::Immediate, 2, 2, "NOP"},
        {OpcodeClass::TXA, 0x8A, AddressingMode::Implied, 1, 2, "TXA"},
        {OpcodeClass::JAM, 0x8B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::STY, 0x8C, AddressingMode::Absolute, 3, 4, "STY"},
        {OpcodeClass::STA, 0x8D, AddressingMode::Absolute, 3, 4, "STA"},
        {OpcodeClass::STX, 0x8E, AddressingMode::Absolute, 3, 4, "STX"},
        {OpcodeClass::JAM, 0x8F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BCC, 0x90, AddressingMode::Relative, 2, 2, "BCC"},
        {OpcodeClass::STA, 0x91, AddressingMode::IndirectY, 2, 6, "STA"},
        {OpcodeClass::JAM, 0x92, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x93, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::STY, 0x94, AddressingMode::ZeroPageX, 2, 4, "STY"},
        {OpcodeClass::STA, 0x95, AddressingMode::ZeroPageX, 2, 4, "STA"},
        {OpcodeClass::STX, 0x96, AddressingMode::ZeroPageY, 2, 4, "STX"},
        {OpcodeClass::JAM, 0x97, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::TYA, 0x98, AddressingMode::Implied, 1, 2, "TYA"},
        {OpcodeClass::STA, 0x99, AddressingMode::AbsoluteYIndexed, 3, 5, "STA"},
        {OpcodeClass::TXS, 0x9A, AddressingMode::Implied, 1, 2, "TXS"},
        {OpcodeClass::JAM, 0x9B, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x9C, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::STA, 0x9D, AddressingMode::AbsoluteXIndexed, 3, 5, "STA"},
        {OpcodeClass::JAM, 0x9E, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0x9F, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::LDY, 0xA0, AddressingMode::Immediate, 2, 2, "LDY"},
        {OpcodeClass::LDA, 0xA1, AddressingMode::IndirectX, 2, 6, "LDA"},
        {OpcodeClass::LDX, 0xA2, AddressingMode::Immediate, 2, 2, "LDX"},
        {OpcodeClass::JAM, 0xA3, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::LDY, 0xA4, AddressingMode::ZeroPage, 2, 3, "LDY"},
        {OpcodeClass::LDA, 0xA5, AddressingMode::ZeroPage, 2, 3, "LDA"},
        {OpcodeClass::LDX, 0xA6, AddressingMode::ZeroPage, 2, 3, "LDX"},
        {OpcodeClass::JAM, 0xA7, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::TAY, 0xA8, AddressingMode::Implied, 1, 2, "TAY"},
        {OpcodeClass::LDA, 0xA9, AddressingMode::Immediate, 2, 2, "LDA"},
        {OpcodeClass::TAX, 0xAA, AddressingMode::Implied, 1, 2, "TAX"},
        {OpcodeClass::JAM, 0xAB, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::LDY, 0xAC, AddressingMode::Absolute, 3, 4, "LDY"},
        {OpcodeClass::LDA, 0xAD, AddressingMode::Absolute, 3, 4, "LDA"},
        {OpcodeClass::LDX, 0xAE, AddressingMode::Absolute, 3, 4, "LDX"},
        {OpcodeClass::JAM, 0xAF, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BCS, 0xB0, AddressingMode::Relative, 2, 2, "BCS"},
        {OpcodeClass::LDA, 0xB1, AddressingMode::IndirectY, 2, 5, "LDA"},
        {OpcodeClass::JAM, 0xB2, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0xB3, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::LDY, 0xB4, AddressingMode::ZeroPageX, 2, 4, "LDY"},
        {OpcodeClass::LDA, 0xB5, AddressingMode::ZeroPageX, 2, 4, "LDA"},
        {OpcodeClass::LDX, 0xB6, AddressingMode::ZeroPageY, 2, 4, "LDX"},
        {OpcodeClass::JAM, 0xB7, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CLV, 0xB8, AddressingMode::Implied, 1, 2, "CLV"},
        {OpcodeClass::LDA, 0xB9, AddressingMode::AbsoluteYIndexed, 3, 4, "LDA"},
        {OpcodeClass::TSX, 0xBA, AddressingMode::Implied, 1, 2, "TSX"},
        {OpcodeClass::JAM, 0xBB, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::LDY, 0xBC, AddressingMode::AbsoluteXIndexed, 3, 4, "LDY"},
        {OpcodeClass::LDA, 0xBD, AddressingMode::AbsoluteXIndexed, 3, 4, "LDA"},
        {OpcodeClass::LDX, 0xBE, AddressingMode::AbsoluteYIndexed, 3, 4, "LDX"},
        {OpcodeClass::JAM, 0xBF, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CPY, 0xC0, AddressingMode::Immediate, 2, 2, "CPY"},
        {OpcodeClass::CMP, 0xC1, AddressingMode::IndirectX, 2, 6, "CMP"},
        {OpcodeClass::NOP, 0xC2, AddressingMode::Immediate, 2, 2, "NOP"},
        {OpcodeClass::JAM, 0xC3, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CPY, 0xC4, AddressingMode::ZeroPage, 2, 3, "CPY"},
        {OpcodeClass::CMP, 0xC5, AddressingMode::ZeroPage, 2, 3, "CMP"},
        {OpcodeClass::DEC, 0xC6, AddressingMode::ZeroPage, 2, 5, "DEC"},
        {OpcodeClass::JAM, 0xC7, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::INY, 0xC8, AddressingMode::Implied, 1, 2, "INY"},
        {OpcodeClass::CMP, 0xC9, AddressingMode::Immediate, 2, 2, "CMP"},
        {OpcodeClass::DEX, 0xCA, AddressingMode::Implied, 1, 2, "DEX"},
        {OpcodeClass::JAM, 0xCB, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CPY, 0xCC, AddressingMode::Absolute, 3, 4, "CPY"},
        {OpcodeClass::CMP, 0xCD, AddressingMode::Absolute, 3, 4, "CMP"},
        {OpcodeClass::DEC, 0xCE, AddressingMode::Absolute, 3, 6, "DEC"},
        {OpcodeClass::JAM, 0xCF, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BNE, 0xD0, AddressingMode::Relative, 2, 2, "BNE"},
        {OpcodeClass::CMP, 0xD1, AddressingMode::IndirectY, 2, 5, "CMP"},
        {OpcodeClass::JAM, 0xD2, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0xD3, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0xD4, AddressingMode::ZeroPageX, 2, 4, "NOP"},
        {OpcodeClass::CMP, 0xD5, AddressingMode::ZeroPageX, 2, 4, "CMP"},
        {OpcodeClass::DEC, 0xD6, AddressingMode::ZeroPageX, 2, 6, "DEC"},
        {OpcodeClass::JAM, 0xD7, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CLD, 0xD8, AddressingMode::Implied, 1, 2, "CLD"},
        {OpcodeClass::CMP, 0xD9, AddressingMode::AbsoluteYIndexed, 3, 4, "CMP"},
        {OpcodeClass::NOP, 0xDA, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0xDB, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0xDC, AddressingMode::AbsoluteXIndexed, 3, 4, "NOP"},
        {OpcodeClass::CMP, 0xDD, AddressingMode::AbsoluteXIndexed, 3, 4, "CMP"},
        {OpcodeClass::DEC, 0xDE, AddressingMode::AbsoluteXIndexed, 3, 7, "DEC"},
        {OpcodeClass::JAM, 0xDF, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CPX, 0xE0, AddressingMode::Immediate, 2, 2, "CPX"},
        {OpcodeClass::SBC, 0xE1, AddressingMode::IndirectX, 2, 6, "SBC"},
        {OpcodeClass::NOP, 0xE2, AddressingMode::Immediate, 2, 2, "NOP"},
        {OpcodeClass::JAM, 0xE3, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CPX, 0xE4, AddressingMode::ZeroPage, 2, 3, "CPX"},
        {OpcodeClass::SBC, 0xE5, AddressingMode::ZeroPage, 2, 3, "SBC"},
        {OpcodeClass::INC, 0xE6, AddressingMode::ZeroPage, 2, 5, "INC"},
        {OpcodeClass::JAM, 0xE7, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::INX, 0xE8, AddressingMode::Implied, 1, 2, "INX"},
        {OpcodeClass::SBC, 0xE9, AddressingMode::Immediate, 2, 2, "SBC"},
        {OpcodeClass::NOP, 0xEA, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0xEB, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::CPX, 0xEC, AddressingMode::Absolute, 3, 4, "CPX"},
        {OpcodeClass::SBC, 0xED, AddressingMode::Absolute, 3, 4, "SBC"},
        {OpcodeClass::INC, 0xEE, AddressingMode::Absolute, 3, 6, "INC"},
        {OpcodeClass::JAM, 0xEF, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::BEQ, 0xF0, AddressingMode::Relative, 2, 2, "BEQ"},
        {OpcodeClass::SBC, 0xF1, AddressingMode::IndirectY, 2, 5, "SBC"},
        {OpcodeClass::JAM, 0xF2, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::JAM, 0xF3, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0xF4, AddressingMode::ZeroPageX, 2, 4, "NOP"},
        {OpcodeClass::SBC, 0xF5, AddressingMode::ZeroPageX, 2, 4, "SBC"},
        {OpcodeClass::INC, 0xF6, AddressingMode::ZeroPageX, 2, 6, "INC"},
        {OpcodeClass::JAM, 0xF7, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::SED, 0xF8, AddressingMode::Implied, 1, 2, "SED"},
        {OpcodeClass::SBC, 0xF9, AddressingMode::AbsoluteYIndexed, 3, 4, "SBC"},
        {OpcodeClass::NOP, 0xFA, AddressingMode::Implied, 1, 2, "NOP"},
        {OpcodeClass::JAM, 0xFB, AddressingMode::Implied, 1, 1, "JAM"},
        {OpcodeClass::NOP, 0xFC, AddressingMode::AbsoluteXIndexed, 3, 4, "NOP"},
        {OpcodeClass::SBC, 0xFD, AddressingMode::AbsoluteXIndexed, 3, 4, "SBC"},
        {OpcodeClass::INC, 0xFE, AddressingMode::AbsoluteXIndexed, 3, 7, "INC"},
        {OpcodeClass::JAM, 0xFF, AddressingMode::Implied, 1, 1, "JAM"},
    },
};

template<SystemBus BusType>
void Cpu<BusType>::start() {
    // The CPU start procedure takes 7 NES cycles
    bus->ticked_cpu_read(0x0000); // Address does not matter
    bus->ticked_cpu_read(0x0001); // First start state
    bus->ticked_cpu_read(0x0100 + s); // Second start state
    bus->ticked_cpu_read(0x0100 + s - 1); // Third start state
    bus->ticked_cpu_read(0x0100 + s - 2); // Fourth start state
    auto pcl = bus->ticked_cpu_read(RESET_VECTOR);
    auto pch = bus->ticked_cpu_read(RESET_VECTOR + 1);
    pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
    spdlog::info("Starting execution at {:#06X}", pc);
};

template<SystemBus BusType>
void Cpu<BusType>::step() {
    check_interrupts();

    const auto initial_cycles = bus->cycles;

    auto opcode = OPCODES[fetch()];

    ExecutedOpcode executed_opcode{
        .start_cycle = initial_cycles,
        .pc = static_cast<word>(pc - 1),
        .opcode = opcode.opcode,
    };
    if (opcode.length >= 2) {
        executed_opcode.arg1 = bus->cpu_read(pc);
    }
    if (opcode.length >= 3) {
        executed_opcode.arg2 = bus->cpu_read(pc + 1);
    }
    executed_opcodes.push_back(executed_opcode);

    execute_opcode(opcode);
};

template<SystemBus BusType>
void Cpu<BusType>::check_interrupts() {
    if (*nmi_requested) {
        *nmi_requested = false;

        bus->ticked_cpu_read(pc);
        bus->ticked_cpu_read(pc);
        bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc >> 8));
        bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc));

        update_flag(StatusFlag::InterruptDisable, true);
        // Ensure the B flag is not set when pushing
        update_flag(StatusFlag::B, false);
        p |= (1 << 5); // The unused flag is set when pushing by NMI
        bus->ticked_cpu_write(0x100 + s--, p);

        auto pcl = bus->ticked_cpu_read(NMI_VECTOR);
        auto pch = bus->ticked_cpu_read(NMI_VECTOR + 1);
        pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
    } else if (!flag_set(StatusFlag::InterruptDisable) && *irq_requested) {
        *irq_requested = false;

        bus->ticked_cpu_read(pc);
        bus->ticked_cpu_read(pc);
        bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc >> 8));
        bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc));

        update_flag(StatusFlag::InterruptDisable, true);
        // Ensure the B flag is not set when pushing
        update_flag(StatusFlag::B, false);
        p |= (1 << 5); // The unused flag is set when pushing by NMI
        bus->ticked_cpu_write(0x100 + s--, p);

        auto pcl = bus->ticked_cpu_read(IRQ_VECTOR);
        auto pch = bus->ticked_cpu_read(IRQ_VECTOR + 1);
        pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
    }
}

template<SystemBus BusType>
void Cpu<BusType>::execute_opcode(Opcode opcode) {
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
template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::absolute_addressing() {
    const auto low = fetch();
    const auto high = fetch();

    return {static_cast<word>(high) << 8 | static_cast<word>(low), false};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::indirect_addressing() {
    auto [pointer, _] = absolute_addressing();

    auto low = bus->ticked_cpu_read(pointer);
    auto high = bus->ticked_cpu_read(non_page_crossing_add(pointer, 1));

    return {static_cast<word>(high) << 8 | static_cast<word>(low), false};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::zero_page_addressing() {
    return {static_cast<word>(fetch()), false};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::zero_page_x_addressing() {
    auto [address, _] = zero_page_addressing();
    bus->tick(); // Dummy read cycle
    return {non_page_crossing_add(address, x), false};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::zero_page_y_addressing() {
    auto [address, _] = zero_page_addressing();
    bus->tick(); // Dummy read cycle
    return {non_page_crossing_add(address, y), false};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::absolute_x_indexed_addressing() {
    auto [address, _] = absolute_addressing();

    bool page_crossed = false;
    if ((address + x) != non_page_crossing_add(address, x)) {
        // If a page was crossed we require an additional cycle
        bus->tick(); // Dummy read cycle
        page_crossed = true;
    }

    return {address + x, page_crossed};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::absolute_y_indexed_addressing() {
    auto [address, _] = absolute_addressing();

    bool page_crossed = false;
    if ((address + y) != non_page_crossing_add(address, y)) {
        // If a page was crossed we require an additional cycle
        bus->tick(); // Dummy read cycle
        page_crossed = true;
    }

    return {address + y, page_crossed};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::indirect_x_addressing() {
    auto operand = static_cast<word>(fetch());
    const auto pointer = operand + x;
    bus->ticked_cpu_read(operand); // Dummy read cycle

    const auto low = static_cast<word>(bus->ticked_cpu_read(pointer & 0xFF));
    const auto high = static_cast<word>(bus->ticked_cpu_read((pointer + 1) & 0xFF));

    return {(high << 8) | low, false};
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::indirect_y_addressing() {
    auto pointer = static_cast<word>(fetch());
    const auto low = static_cast<word>(bus->ticked_cpu_read(pointer));
    const auto high = static_cast<word>(bus->ticked_cpu_read((pointer + 1) & 0xFF));

    const word effective = ((high << 8) | low);
    word non_page_crossed_address = non_page_crossing_add(effective, static_cast<word>(y));
    word page_crossed_address = effective + static_cast<word>(y);
    bus->ticked_cpu_read(non_page_crossed_address);

    if (non_page_crossed_address != page_crossed_address) {
        return {page_crossed_address, true};
    } else {
        return {non_page_crossed_address, false};
    }
}

template<SystemBus BusType>
EffectiveAddress Cpu<BusType>::fetch_effective_address(AddressingMode mode) {
    switch (mode) {
        case AddressingMode::Immediate:
            return {pc++, false};
        case AddressingMode::ZeroPage:
            return zero_page_addressing();
        case AddressingMode::ZeroPageX:
            return zero_page_x_addressing();
        case AddressingMode::ZeroPageY:
            return zero_page_y_addressing();
        case AddressingMode::Absolute:
            return absolute_addressing();
        case AddressingMode::AbsoluteXIndexed:
            return absolute_x_indexed_addressing();
        case AddressingMode::AbsoluteYIndexed:
            return absolute_y_indexed_addressing();
        case AddressingMode::Indirect:
            return indirect_addressing();
        case AddressingMode::IndirectX:
            return indirect_x_addressing();
        case AddressingMode::IndirectY:
            return indirect_y_addressing();
        default:
            spdlog::error("Invalid addressing mode for effective address");
            std::exit(-1);
    }
}

// Opcodes
template<SystemBus BusType>
void Cpu<BusType>::BRK(const Opcode&) {
    fetch();
    bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc >> 8));
    bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc));

    // NMI and IRQ can hijack BRK interrupt if they are pending here
    // TODO: Implement IRQ interrupt although it should not matter since both
    //       use the same vector
    auto interrupt_vector = *nmi_requested ? NMI_VECTOR : IRQ_VECTOR;
    byte temp_p = p | static_cast<byte>(StatusFlag::B); // Ensure bits 45 are set before push
    bus->ticked_cpu_write(0x100 + s--, temp_p);
    update_flag(StatusFlag::InterruptDisable, true);

    auto pcl = bus->ticked_cpu_read(interrupt_vector);
    auto pch = bus->ticked_cpu_read(interrupt_vector + 1);
    pc = (static_cast<word>(pch) << 8) | static_cast<word>(pcl);
}

template<SystemBus BusType>
void Cpu<BusType>::JMP(const Opcode& opcode) {
    auto [address, _] = fetch_effective_address(opcode.addressing_mode);
    pc = address;
}

template<SystemBus BusType>
void Cpu<BusType>::LDX(const Opcode& opcode) {
    auto [address, _] = fetch_effective_address(opcode.addressing_mode);
    x = bus->ticked_cpu_read(address);
    update_flag(StatusFlag::Zero, x == 0x00);
    update_flag(StatusFlag::Negative, (x & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::STX(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
         || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
        && !page_crossed) {
        bus->ticked_cpu_read(address); // Dummy read cycle
    }
    bus->ticked_cpu_write(address, x);
}

template<SystemBus BusType>
void Cpu<BusType>::JSR(const Opcode&) {
    const auto low = static_cast<word>(fetch());

    bus->ticked_cpu_read(0x100 + s); // Dummy read cycle (3)

    bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc >> 8));
    bus->ticked_cpu_write(0x100 + s--, static_cast<byte>(pc));

    const word high = static_cast<word>(fetch()) << 8;

    pc = high | low;
}

template<SystemBus BusType>
void Cpu<BusType>::RTS(const Opcode&) {
    bus->ticked_cpu_read(pc); // Fetch next opcode and discard it

    bus->ticked_cpu_read(0x100 + s++); // Dummy read cycle (3)

    auto low = bus->ticked_cpu_read(0x100 + s++);
    auto high = bus->ticked_cpu_read(0x100 + s);
    pc = (high << 8) | low;

    bus->ticked_cpu_read(pc++);
}

template<SystemBus BusType>
void Cpu<BusType>::NOP(const Opcode& opcode) {
    if (opcode.addressing_mode != AddressingMode::Implied) {
        auto [address, _] = fetch_effective_address(opcode.addressing_mode);
        bus->ticked_cpu_read(address);
    } else {
        bus->tick();
    }
}

template<SystemBus BusType>
void Cpu<BusType>::JAM(const Opcode&) {
    // These two reads are based on ProcessorTests
    bus->ticked_cpu_read(pc);
    bus->ticked_cpu_read(pc);
    pc--; // The PC should not be incremented after a JAM opcode
}

template<SystemBus BusType>
void Cpu<BusType>::SEC(const Opcode&) {
    update_flag(StatusFlag::Carry, true);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::CLC(const Opcode&) {
    update_flag(StatusFlag::Carry, false);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::CLD(const Opcode&) {
    update_flag(StatusFlag::Decimal, false);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::CLV(const Opcode&) {
    update_flag(StatusFlag::Overflow, false);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::CLI(const Opcode&) {
    update_flag(StatusFlag::InterruptDisable, false);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::SEI(const Opcode&) {
    update_flag(StatusFlag::InterruptDisable, true);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::SED(const Opcode&) {
    update_flag(StatusFlag::Decimal, true);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::BCC(const Opcode&) {
    relative_branch_on(!flag_set(StatusFlag::Carry));
}

template<SystemBus BusType>
void Cpu<BusType>::BCS(const Opcode&) {
    relative_branch_on(flag_set(StatusFlag::Carry));
}

template<SystemBus BusType>
void Cpu<BusType>::BEQ(const Opcode&) {
    relative_branch_on(flag_set(StatusFlag::Zero));
}

template<SystemBus BusType>
void Cpu<BusType>::BNE(const Opcode&) {
    relative_branch_on(!flag_set(StatusFlag::Zero));
}

template<SystemBus BusType>
void Cpu<BusType>::BMI(const Opcode&) {
    relative_branch_on(flag_set(StatusFlag::Negative));
}

template<SystemBus BusType>
void Cpu<BusType>::BPL(const Opcode&) {
    relative_branch_on(!flag_set(StatusFlag::Negative));
}

template<SystemBus BusType>
void Cpu<BusType>::BVC(const Opcode&) {
    relative_branch_on(!flag_set(StatusFlag::Overflow));
}

template<SystemBus BusType>
void Cpu<BusType>::BVS(const Opcode&) {
    relative_branch_on(flag_set(StatusFlag::Overflow));
}

template<SystemBus BusType>
void Cpu<BusType>::LDA(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    if (opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        a = bus->cpu_read(address);
    } else {
        a = bus->ticked_cpu_read(address);
    }
    update_flag(StatusFlag::Zero, a == 0x00);
    update_flag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::STA(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
         || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
        && !page_crossed) {
        bus->ticked_cpu_read(address); // Dummy read cycle
    }

    bus->ticked_cpu_write(address, a);
}

template<SystemBus BusType>
void Cpu<BusType>::STY(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
         || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
        && !page_crossed) {
        bus->ticked_cpu_read(address); // Dummy read cycle
    }
    bus->ticked_cpu_write(address, y);
}

template<SystemBus BusType>
void Cpu<BusType>::BIT(const Opcode& opcode) {
    auto [address, _] = fetch_effective_address(opcode.addressing_mode);
    auto operand = bus->ticked_cpu_read(address);

    update_flag(StatusFlag::Negative, (operand & 0x80) != 0x00);
    update_flag(StatusFlag::Overflow, (operand & 0x40) != 0x00);
    update_flag(StatusFlag::Zero, (operand & a) == 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::PHA(const Opcode&) {
    bus->ticked_cpu_read(pc); // Fetch next opcode and discard it
    bus->ticked_cpu_write(0x100 + s--, a);
}

template<SystemBus BusType>
void Cpu<BusType>::PLA(const Opcode&) {
    bus->ticked_cpu_read(pc); // Fetch next opcode and discard it
    bus->ticked_cpu_read(0x100 + s++); // Dummy read cycle (3)
    a = bus->ticked_cpu_read(0x100 + s);
    update_flag(StatusFlag::Zero, a == 0x00);
    update_flag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::PHP(const Opcode&) {
    bus->ticked_cpu_read(pc); // Fetch next opcode and discard it
    byte temp_p = p | static_cast<byte>(StatusFlag::B); // Ensure bits 45 are set before push
    bus->ticked_cpu_write(0x100 + s--, temp_p);
}

template<SystemBus BusType>
void Cpu<BusType>::PLP(const Opcode&) {
    bus->ticked_cpu_read(pc); // Fetch next opcode and discard it
    bus->ticked_cpu_read(0x100 + s++); // Dummy read cycle (3)
    auto temp_p = bus->ticked_cpu_read(0x100 + s);
    // Bits 54 of the popped value from the stack should be ignored
    p = (p & 0x30) | (temp_p & 0xCF);
}

template<SystemBus BusType>
void Cpu<BusType>::AND(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    byte operand;
    if (opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        operand = bus->cpu_read(address);
    } else {
        operand = bus->ticked_cpu_read(address);
    }
    a = a & operand;
    update_flag(StatusFlag::Zero, a == 0x00);
    update_flag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::ORA(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    byte operand;
    if (opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        operand = bus->cpu_read(address);
    } else {
        operand = bus->ticked_cpu_read(address);
    }
    a = a | operand;
    update_flag(StatusFlag::Zero, a == 0x00);
    update_flag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::EOR(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    byte operand;
    if (opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        operand = bus->cpu_read(address);
    } else {
        operand = bus->ticked_cpu_read(address);
    }
    a = a ^ operand;
    update_flag(StatusFlag::Zero, a == 0x00);
    update_flag(StatusFlag::Negative, (a & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::CMP(const Opcode& opcode) {
    cmp_reg_and_mem(opcode, a);
}

template<SystemBus BusType>
void Cpu<BusType>::CPX(const Opcode& opcode) {
    cmp_reg_and_mem(opcode, x);
}

template<SystemBus BusType>
void Cpu<BusType>::CPY(const Opcode& opcode) {
    cmp_reg_and_mem(opcode, y);
}

template<SystemBus BusType>
void Cpu<BusType>::ADC(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    word operand;
    if (opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        operand = static_cast<word>(bus->cpu_read(address));
    } else {
        operand = static_cast<word>(bus->ticked_cpu_read(address));
    }

    auto temp_a = static_cast<word>(a);
    word result = temp_a + operand + static_cast<word>(p & static_cast<byte>(StatusFlag::Carry));

    update_flag(StatusFlag::Zero, (result & 0xFF) == 0x00);
    update_flag(StatusFlag::Negative, (result & 0x80) != 0x00);
    update_flag(StatusFlag::Carry, result > 0xFF);
    // Reference:
    // https://github.com/OneLoneCoder/olcNES/blob/663e3777191c011135dfb6d40c887ae126970dd7/Part%20%233%20-%20Buses%2C%20Rams%2C%20Roms%20%26%20Mappers/olc6502.cpp#L589
    // http://www.6502.org/tutorials/vflag.html
    update_flag(StatusFlag::Overflow, ((~(temp_a ^ operand) & (temp_a ^ result)) & 0x80) != 0x00);

    a = static_cast<byte>(result & 0xFF);
}

template<SystemBus BusType>
void Cpu<BusType>::SBC(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    // Fetch operand and invert bottom 8 bits
    word operand;
    if (opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        operand = static_cast<word>(bus->cpu_read(address));
    } else {
        operand = static_cast<word>(bus->ticked_cpu_read(address));
    }
    operand ^= 0x00FF;

    const auto temp_a = static_cast<word>(a);
    const word result =
        temp_a + operand + static_cast<word>(p & static_cast<byte>(StatusFlag::Carry));

    update_flag(StatusFlag::Zero, (result & 0xFF) == 0x00);
    update_flag(StatusFlag::Negative, (result & 0x80) != 0x00);
    update_flag(StatusFlag::Carry, result > 0xFF);
    // Reference:
    // https://github.com/OneLoneCoder/olcNES/blob/663e3777191c011135dfb6d40c887ae126970dd7/Part%20%233%20-%20Buses%2C%20Rams%2C%20Roms%20%26%20Mappers/olc6502.cpp#L589
    // http://www.6502.org/tutorials/vflag.html
    update_flag(StatusFlag::Overflow, ((~(temp_a ^ operand) & (temp_a ^ result)) & 0x80) != 0x00);

    a = static_cast<byte>(result);
}

template<SystemBus BusType>
void Cpu<BusType>::LDY(const Opcode& opcode) {
    auto [address, _] = fetch_effective_address(opcode.addressing_mode);
    y = bus->ticked_cpu_read(address);
    update_flag(StatusFlag::Zero, y == 0x00);
    update_flag(StatusFlag::Negative, (y & 0x80) != 0x00);
}

template<SystemBus BusType>
void Cpu<BusType>::INY(const Opcode&) {
    y++;
    update_flag(StatusFlag::Zero, y == 0x00);
    update_flag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::INX(const Opcode&) {
    x++;
    update_flag(StatusFlag::Zero, x == 0x00);
    update_flag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::INC(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
         || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
        && !page_crossed) {
        bus->ticked_cpu_read(address); // Dummy read cycle
    }
    byte operand = bus->ticked_cpu_read(address);

    bus->ticked_cpu_write(address, operand++); // Dummy write cycle
    update_flag(StatusFlag::Zero, operand == 0x00);
    update_flag(StatusFlag::Negative, (operand & 0x80) != 0x00);

    bus->ticked_cpu_write(address, operand);
}

template<SystemBus BusType>
void Cpu<BusType>::DEC(const Opcode& opcode) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
         || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
        && !page_crossed) {
        bus->ticked_cpu_read(address); // Dummy read cycle
    }
    byte operand = bus->ticked_cpu_read(address);

    bus->ticked_cpu_write(address, operand--); // Dummy write cycle
    update_flag(StatusFlag::Zero, operand == 0x00);
    update_flag(StatusFlag::Negative, (operand & 0x80) != 0x00);

    bus->ticked_cpu_write(address, operand);
}

template<SystemBus BusType>
void Cpu<BusType>::DEY(const Opcode&) {
    y--;
    update_flag(StatusFlag::Zero, y == 0x00);
    update_flag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::DEX(const Opcode&) {
    x--;
    update_flag(StatusFlag::Zero, x == 0x00);
    update_flag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::TAX(const Opcode&) {
    x = a;
    update_flag(StatusFlag::Zero, x == 0x00);
    update_flag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::TAY(const Opcode&) {
    y = a;
    update_flag(StatusFlag::Zero, y == 0x00);
    update_flag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::TSX(const Opcode&) {
    x = s;
    update_flag(StatusFlag::Zero, x == 0x00);
    update_flag(StatusFlag::Negative, (x & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::TXA(const Opcode&) {
    a = x;
    update_flag(StatusFlag::Zero, a == 0x00);
    update_flag(StatusFlag::Negative, (a & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::TXS(const Opcode&) {
    s = x;
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::TYA(const Opcode&) {
    a = y;
    update_flag(StatusFlag::Zero, y == 0x00);
    update_flag(StatusFlag::Negative, (y & 0x80) != 0x00);
    bus->tick();
}

template<SystemBus BusType>
void Cpu<BusType>::RTI(const Opcode&) {
    bus->ticked_cpu_read(pc); // Fetch next opcode and discard it
    bus->ticked_cpu_read(0x100 + (s++)); // Dummy read cycle
    auto temp_p = bus->ticked_cpu_read(0x100 + s++);
    // Bits 54 of the popped value from the stack should be ignored
    p = (p & 0x30) | (temp_p & 0xCF);

    const auto low = static_cast<word>(bus->ticked_cpu_read(0x100 + s++));
    const auto high = static_cast<word>(bus->ticked_cpu_read(0x100 + s));

    pc = (high << 8) | low;
}

template<SystemBus BusType>
void Cpu<BusType>::LSR(const Opcode& opcode) {
    byte operand;
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
             || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
            && !page_crossed) {
            bus->ticked_cpu_read(address); // Dummy read cycle
        }
        operand = bus->ticked_cpu_read(address);
    }

    byte result = operand >> 1;
    update_flag(StatusFlag::Negative, false);
    update_flag(StatusFlag::Zero, result == 0x00);
    update_flag(StatusFlag::Carry, (operand & 0b1) != 0x00);
    bus->tick(); // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->ticked_cpu_write(address, result);
    }
}

template<SystemBus BusType>
void Cpu<BusType>::ASL(const Opcode& opcode) {
    byte operand;
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
             || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
            && !page_crossed) {
            bus->ticked_cpu_read(address); // Dummy read cycle
        }
        operand = bus->ticked_cpu_read(address);
    }

    byte result = operand << 1;
    update_flag(StatusFlag::Negative, (result & 0x80) != 0);
    update_flag(StatusFlag::Zero, result == 0x00);
    update_flag(StatusFlag::Carry, (operand & 0x80) != 0x00);
    bus->tick(); // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->ticked_cpu_write(address, result);
    }
}

template<SystemBus BusType>
void Cpu<BusType>::ROL(const Opcode& opcode) {
    byte operand;
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
             || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
            && !page_crossed) {
            bus->ticked_cpu_read(address); // Dummy read cycle
        }
        operand = bus->ticked_cpu_read(address);
    }

    byte result = (operand << 1) | (flag_set(StatusFlag::Carry) ? 0b1 : 0b0);
    update_flag(StatusFlag::Negative, (result & 0x80) != 0);
    update_flag(StatusFlag::Zero, result == 0x00);
    update_flag(StatusFlag::Carry, (operand & 0x80) != 0x00);
    bus->tick(); // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->ticked_cpu_write(address, result);
    }
}

template<SystemBus BusType>
void Cpu<BusType>::ROR(const Opcode& opcode) {
    byte operand;
    word address{};
    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        operand = a;
    } else {
        auto [temp_address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
        address = temp_address;
        if ((opcode.addressing_mode == AddressingMode::AbsoluteXIndexed
             || opcode.addressing_mode == AddressingMode::AbsoluteYIndexed)
            && !page_crossed) {
            bus->ticked_cpu_read(address); // Dummy read cycle
        }
        operand = bus->ticked_cpu_read(address);
    }

    byte result = (operand >> 1) | (flag_set(StatusFlag::Carry) ? 0x80 : 0x00);
    update_flag(StatusFlag::Negative, (result & 0x80) != 0);
    update_flag(StatusFlag::Zero, result == 0x00);
    update_flag(StatusFlag::Carry, (operand & 0b1) != 0x00);
    bus->tick(); // Dummy read cycle

    if (opcode.addressing_mode == AddressingMode::Accumulator) {
        a = result;
    } else {
        bus->ticked_cpu_write(address, result);
    }
}

// Opcode helpers

template<SystemBus BusType>
void Cpu<BusType>::relative_branch_on(const bool condition) {
    // First change the type, then the size, then type again
    const auto offset = static_cast<word>(static_cast<int16_t>(static_cast<int8_t>(fetch())));

    if (!condition)
        return;

    bus->tick(); // Cycle 3 if branch taken

    const word page_crossed = pc + offset;
    const word non_page_crossed = non_page_crossing_add(pc, offset);

    pc = non_page_crossed;

    if (page_crossed != non_page_crossed) {
        bus->tick(); // Cycle 4 for page crossing
        pc = page_crossed;
    }
}

template<SystemBus BusType>
void Cpu<BusType>::cmp_reg_and_mem(const Opcode& opcode, byte reg) {
    auto [address, page_crossed] = fetch_effective_address(opcode.addressing_mode);
    word operand;
    if (opcode.opcode_class == OpcodeClass::CMP
        && opcode.addressing_mode == AddressingMode::IndirectY && !page_crossed) {
        operand = static_cast<word>(bus->cpu_read(address));
    } else {
        operand = static_cast<word>(bus->ticked_cpu_read(address));
    }

    const byte result = reg - operand;
    update_flag(StatusFlag::Zero, result == 0x00);
    update_flag(StatusFlag::Negative, (result & 0x80) != 0x00);
    // The carry flag is set if no borrow or greater than or equal for A - M
    update_flag(StatusFlag::Carry, reg >= operand);
}
