#pragma once

#include <array>
#include <memory>
#include <span>

#include "constants.hxx"
#include "cpu.hxx"
#include "sen.hxx"
#include "util.hxx"

struct CpuState {
    byte& a;
    byte& x;
    byte& y;
    byte& s;
    word& pc;
    byte& p;
    FixedSizeQueue<ExecutedOpcode> executed_opcodes;
};

struct PatternTablesState {
    std::span<byte, 4096> left;
    std::span<byte, 4096> right;
};

class Debugger {
   private:
    std::shared_ptr<Sen> emulator_context{};

   public:
    Debugger() = default;
    Debugger(std::shared_ptr<Sen> emulator_context)
        : emulator_context{std::move(emulator_context)} {}

    template <typename BusType>
    static CpuState GetCpuState(Cpu<BusType>& cpu) {
        return CpuState{
            .a = cpu.a,
            .x = cpu.x,
            .y = cpu.y,
            .s = cpu.s,
            .pc = cpu.pc,
            .p = cpu.p,
            .executed_opcodes = cpu.executed_opcodes,
        };
    }
    CpuState GetCpuState() { return GetCpuState(this->emulator_context->cpu); }
    CpuState GetCpuState() const { return GetCpuState(this->emulator_context->cpu); }

    PatternTablesState GetPatternTableState() const {
        auto chr_mem = emulator_context->bus->cartridge->chr_rom;
        return {.left = std::span<byte, 4096>{&chr_mem[0x0000], 0x1000},
                .right = std::span<byte, 4096>{&chr_mem[0x1000], 0x1000}};
    }

    void GetPpuState() const {}
    void GetCartridgeInfo() const {}
};
