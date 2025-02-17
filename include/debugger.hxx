#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "constants.hxx"
#include "cpu.hxx"
#include "ppu.hxx"
#include "sen.hxx"
#include "util.hxx"

struct CpuState {
    byte& a;
    byte& x;
    byte& y;
    byte& s;
    word& pc;
    byte& p;
};

struct SpriteData {
    Sprite oam_entry;
    std::array<byte, 16> tile_data;
};

struct Sprites {
    std::array<SpriteData, 64> sprites_data;
    std::array<byte, 0x20> palettes;
};

struct PpuState {
    uint64_t frame_count;
    unsigned int scanline;
    unsigned int line_cycles;

    word v;
    word t;
    byte ppuctrl;
    byte ppumask;
    byte ppustatus;
    byte oamaddr;
};

struct PatternTablesState {
    std::array<byte, 0x1000> left;
    std::array<byte, 0x1000> right;
    std::array<byte, 0x20> palettes;
};

struct PpuMemory {
    std::vector<byte> ppu_memory;
};

class Debugger {
    std::shared_ptr<Sen> emulator_context{};

  public:
    Debugger() = default;

    explicit Debugger(std::shared_ptr<Sen> emulator_context) :
        emulator_context{std::move(emulator_context)} {}

    [[nodiscard]] std::span<word, NES_WIDTH * NES_HEIGHT> Framebuffer() const {
        return std::span<word, NES_WIDTH * NES_HEIGHT>{
            emulator_context->ppu->framebuffer.data(),
            NES_WIDTH * NES_HEIGHT
        };
    }

    template<typename BusType>
    static CpuState GetCpuState(Cpu<BusType>& cpu) {
        return CpuState{
            .a = cpu.a,
            .x = cpu.x,
            .y = cpu.y,
            .s = cpu.s,
            .pc = cpu.pc,
            .p = cpu.p,
        };
    }

    CpuState GetCpuState() {
        return GetCpuState(this->emulator_context->cpu);
    }

    [[nodiscard]] CpuState GetCpuState() const {
        return GetCpuState(this->emulator_context->cpu);
    }

    template<typename BusType>
    static const FixedSizeQueue<ExecutedOpcode>& GetCpuExecutedOpcodes(const Cpu<BusType>& cpu) {
        return cpu.executed_opcodes;
    }

    [[nodiscard]] const FixedSizeQueue<ExecutedOpcode>& GetCpuExecutedOpcodes() const {
        return GetCpuExecutedOpcodes(this->emulator_context->cpu);
    }

    void load_sprite_data(Sprites& sprites) const {
        const auto& ppu = emulator_context->ppu;
        const auto& cart = ppu->cartridge;
        const auto& oam = ppu->oam;

        sprites.sprites_data = {};
        for (size_t i = 0; i < 0x20; i++) {
            sprites.palettes.at(i) = ppu->palette_table.at(i);
        }

        const word sprite_pattern_table_address = ppu->SpritePatternTableAddress();

        for (size_t i = 0; i < 64; i++) {
            sprites.sprites_data[i].oam_entry = ppu->oam[i];
            for (size_t j = 0; j < 16; j++) {
                sprites.sprites_data[i].tile_data[j] = cart->ppu_read(sprite_pattern_table_address + (sprites.sprites_data[i].oam_entry.tile_index << 4) + j);
            }
        }
    }

    void load_ppu_state(PpuState& ppu_state) const {
        const auto& ppu = emulator_context->ppu;
        ppu_state.frame_count = ppu->frame_count;
        ppu_state.scanline = ppu->scanline;
        ppu_state.line_cycles = ppu->line_cycles;
        ppu_state.v = ppu->v.value;
        ppu_state.t = ppu->t.value;
        ppu_state.ppuctrl = ppu->ppuctrl;
        ppu_state.ppumask = ppu->ppumask;
        ppu_state.ppustatus = ppu->ppustatus;
        ppu_state.oamaddr = ppu->oamaddr;
    }

    void load_pattern_table_state(PatternTablesState& pattern_tables_state) const {
        const auto& cart = emulator_context->bus->cartridge;
        const auto& palette_table = emulator_context->ppu->palette_table;

        for (size_t i = 0; i < 0x1000; i++) {
            pattern_tables_state.left.at(i) = cart->ppu_read(i);
            pattern_tables_state.right.at(i) = cart->ppu_read(0x1000 + i);
        }

        for (size_t i = 0; i < 32; i++) {
            pattern_tables_state.palettes[i] = palette_table[i];
        }
    }

    void LoadPpuMemory(std::vector<byte>& buffer) const {
        for (word i = 0; i < 0x4000; i++) {
            buffer[i] = emulator_context->ppu->PpuRead(i);
        }
    }
};
