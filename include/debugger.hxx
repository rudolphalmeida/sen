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

struct Sprites {
    std::array<SpriteData, 64> sprites_data;
    std::span<byte, 0x20> palettes;
};

struct PpuState {
    std::span<byte, 0x20> palettes;

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
    std::span<const byte, 4096> left;
    std::span<const byte, 4096> right;

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

    [[nodiscard]] std::span<word, NES_WIDTH * NES_HEIGHT> Framebuffer() const {
        return std::span<word, NES_WIDTH * NES_HEIGHT>{emulator_context->ppu->framebuffer.data(),
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

    static Sprites GetSprites(const std::shared_ptr<Ppu>& ppu) {
        Sprites sprites{.sprites_data = {}, .palettes = std::span<byte, 0x20>{&ppu->palette_table[0], 0x20}};
        auto chr_mem = ppu->cartridge->chr_rom_ref();
        word sprite_pattern_table_address = ppu->SpritePatternTableAddress();
        std::ranges::transform(ppu->oam, sprites.sprites_data.begin(), [&](Sprite sprite) -> SpriteData {
            std::array<byte, 16> tile_data{};
            std::ranges::copy(
                &chr_mem[sprite_pattern_table_address + (sprite.tile_index << 4)],
                &chr_mem[sprite_pattern_table_address + (sprite.tile_index << 4) + 16],
                tile_data.begin());
            return {.oam_entry = sprite, .tile_data=tile_data};
        });

        return sprites;
    }
    Sprites GetSprites() { return GetSprites(this->emulator_context->ppu); }
    [[nodiscard]] Sprites GetSprites() const { return GetSprites(this->emulator_context->ppu); }


    static PpuState GetPpuState(const std::shared_ptr<Ppu>& ppu) {
        auto chr_mem = ppu->cartridge->chr_rom_ref();
        return PpuState{
            .palettes = std::span<byte, 0x20>{&ppu->palette_table[0], 0x20},
            .frame_count = ppu->frame_count,
            .scanline = ppu->scanline,
            .line_cycles = ppu->line_cycles,
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
        const auto chr_mem = emulator_context->bus->cartridge->chr_rom_ref();
        return {.left = std::span(chr_mem.first<0x1000>()),
                .right = std::span(chr_mem.subspan(0x1000).first<0x1000>()),
                .palettes = std::span<byte, 32>{&emulator_context->ppu->palette_table[0], 32}};
    }

    void LoadPpuMemory(std::vector<byte>& buffer) const {
        for (word i = 0; i < 0x4000; i++) {
            buffer[i] = emulator_context->ppu->PpuRead(i);
        }
    }

    void GetCartridgeInfo() const {}
};
