#pragma once

#include <memory>

#include "sen.hxx"
#include "constants.hxx"

struct CpuState {
    byte& a;
    byte& x;
    byte& y;
    byte& s;
    word& pc;

    byte& p;
};

class Debugger {
   private:
    std::shared_ptr<Sen> emulator_context{};

    public:
    Debugger() = default;
     Debugger(std::shared_ptr<Sen> emulator_context)
         : emulator_context{std::move(emulator_context)} {}

    CpuState GetCpuState() {
         return CpuState {
             .a = emulator_context->cpu.a,
             .x = emulator_context->cpu.x,
             .y = emulator_context->cpu.y,
             .s = emulator_context->cpu.s,
             .pc = emulator_context->cpu.pc,
             .p = emulator_context->cpu.p,
         };
    }
    void GetPpuState() const {}
    void GetCartridgeInfo() const {}
};
