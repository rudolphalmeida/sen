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

struct PpuState {
    byte& ppuctrl;
    byte& ppumask;
    byte& ppustatus;
    byte& oamaddr;

    word& v;
    word& t;

    byte& tile_id_latch;
    byte& bg_pattern_msb_latch, bg_pattern_lsb_latch;
    word& bg_pattern_msb_shift_reg, bg_pattern_lsb_shift_reg;
    byte& bg_attrib_latch;
    byte& bg_attrib_msb_shift_reg, bg_attrib_lsb_shift_reg;
    uint64_t& frame_count;
    unsigned int& scanline;
    unsigned int& cycles_into_scanline;

    std::shared_ptr<bool> nmi_requested{};
};

struct PatternTablesState {
    std::span<byte, 4096> left;
    std::span<byte, 4096> right;

    std::span<byte, 0x20> palettes;
};

struct PpuMemory {
    std::vector<byte> ppu_memory;
};

class Debugger {
   private:
    std::shared_ptr<Sen> emulator_context{};

   public:
    Debugger() = default;
    explicit Debugger(std::shared_ptr<Sen> emulator_context)
        : emulator_context{std::move(emulator_context)} {}

    [[nodiscard]] std::span<byte, NES_WIDTH * NES_HEIGHT> Framebuffer() const {
        return std::span<byte, NES_WIDTH * NES_HEIGHT>{emulator_context->ppu->framebuffer.data(), NES_WIDTH * NES_HEIGHT};
    }

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
    [[nodiscard]] CpuState GetCpuState() const { return GetCpuState(this->emulator_context->cpu); }

    static PpuState GetPpuState(const std::shared_ptr<Ppu>& ppu) {
        return PpuState{
            .ppuctrl = ppu->ppuctrl,
            .ppumask = ppu->ppumask,
            .ppustatus = ppu->ppustatus,
            .oamaddr = ppu->oamaddr,
            .v = ppu->v.value,
            .t = ppu->t.value,
            .tile_id_latch = ppu->tile_id_latch,
            .bg_pattern_msb_latch = ppu->bg_pattern_msb_latch,
            .bg_pattern_lsb_latch = ppu->bg_pattern_lsb_latch,
            .bg_pattern_msb_shift_reg = ppu->bg_pattern_msb_shift_reg,
            .bg_pattern_lsb_shift_reg = ppu->bg_pattern_lsb_shift_reg,
            .bg_attrib_latch = ppu->bg_attrib_latch,
            .bg_attrib_msb_shift_reg = ppu->bg_attrib_msb_shift_reg,
            .bg_attrib_lsb_shift_reg = ppu->bg_attrib_lsb_shift_reg,
            .frame_count = ppu->frame_count,
            .scanline = ppu->scanline,
            .cycles_into_scanline = ppu->line_cycles,
            .nmi_requested = ppu->nmi_requested,
        };
    }
    PpuState GetPpuState() { return GetPpuState(this->emulator_context->ppu); }
    [[nodiscard]] PpuState GetPpuState() const { return GetPpuState(this->emulator_context->ppu); }

    [[nodiscard]] PatternTablesState GetPatternTableState() const {
        auto chr_mem = emulator_context->bus->cartridge->chr_rom;
        auto framebuffer = emulator_context->ppu->framebuffer;
        return {.left = std::span<byte, 4096>{&chr_mem[0x0000], 0x1000},
                .right = std::span<byte, 4096>{&chr_mem[0x1000], 0x1000},
                .palettes = std::span<byte, 0x20>{&emulator_context->ppu->palette_table[0], 0x20}};
    }

    void LoadPpuMemory(std::vector<byte>& buffer) const {
        for (word i = 0; i < 0x4000; i++) {
            buffer[i] = emulator_context->ppu->PpuRead(i);
        }
    }

    void GetCartridgeInfo() const {}
};
