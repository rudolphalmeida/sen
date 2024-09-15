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
};

struct SpriteData {
    Sprite oam_entry;
    std::array<byte, 16> tile_data;
};

struct PpuState {
    // std::array<SpriteData, 64> sprite_data;
    std::span<byte, 0x20> palettes;
    uint64_t frame_count;
    word v;
    word t;
    byte ppuctrl;
    byte ppumask;
    byte ppustatus;
    byte oamaddr;
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
        return std::span<byte, NES_WIDTH * NES_HEIGHT>{emulator_context->ppu->framebuffer.data(),
                                                       NES_WIDTH * NES_HEIGHT};
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
        };
    }
    CpuState GetCpuState() { return GetCpuState(this->emulator_context->cpu); }
    [[nodiscard]] CpuState GetCpuState() const { return GetCpuState(this->emulator_context->cpu); }

    template <typename BusType>
    static const FixedSizeQueue<ExecutedOpcode>& GetCpuExecutedOpcodes(const Cpu<BusType>& cpu) {
        return cpu.executed_opcodes;
    }

    [[nodiscard]] const FixedSizeQueue<ExecutedOpcode>& GetCpuExecutedOpcodes() const {
        return GetCpuExecutedOpcodes(this->emulator_context->cpu);
    }


    static PpuState GetPpuState(const std::shared_ptr<Ppu>& ppu) {
        // std::array<SpriteData, 64> sprite_data{};

        // word sprite_pattern_table_address = ppu->SpritePatternTableAddress();
        auto chr_mem = ppu->cartridge->chr_rom;

        // std::ranges::transform(ppu->oam, sprite_data.begin(), [&](Sprite sprite) -> SpriteData {
        //     std::array<byte, 16> tile_data{};
        //     std::ranges::copy(
        //         &chr_mem[sprite_pattern_table_address + (sprite.tile_index << 4)],
        //         &chr_mem[sprite_pattern_table_address + (sprite.tile_index << 4) + 16],
        //         tile_data.begin());
        //     return {.oam_entry = sprite, .tile_data=tile_data};
        // });

        return PpuState{
            // .sprite_data = sprite_data,
            .palettes = std::span<byte, 0x20>{&ppu->palette_table[0], 0x20},
            .frame_count = ppu->frame_count,
            .v = ppu->v.value,
            .t = ppu->t.value,
            .ppuctrl = ppu->ppuctrl,
            .ppumask = ppu->ppumask,
            .ppustatus = ppu->ppustatus,
            .oamaddr = ppu->oamaddr,
        };
    }
    PpuState GetPpuState() { return GetPpuState(this->emulator_context->ppu); }
    [[nodiscard]] PpuState GetPpuState() const { return GetPpuState(this->emulator_context->ppu); }

    [[nodiscard]] PatternTablesState GetPatternTableState() const {
        auto chr_mem = emulator_context->bus->cartridge->chr_rom;
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
