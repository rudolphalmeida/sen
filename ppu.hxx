#pragma once

#include <memory>
#include <optional>

#include <spdlog/spdlog.h>

#include "cartridge.hxx"
#include "constants.hxx"

struct PpuAddrLatch {
    bool high_byte{true};
    word addr{};

    operator word() const { return addr; }
    void ResetLatch() { high_byte = true; }

    void WriteByte(byte value) {
        if (high_byte) {
            addr = (addr & 0x00FF) | (static_cast<word>(value) << 8);
        } else {
            addr = (addr & 0xFF00) | static_cast<word>(value);
        }
        high_byte = !high_byte;
        addr &= 0x3FFF;  // Addresses are mirrored after 0x3FFF
    }

    void IncrementBy(word value) { addr = (addr + value) & 0x3FFF; }
};

struct PpuScrollLatch {
    bool write_x{true};
    byte x_scroll{}, y_scroll{};

    void ResetLatch() { write_x = true; }
    void WriteByte(byte value) {
        if (write_x) {
            x_scroll = value;
        } else {
            y_scroll = value;
        }
        write_x = !write_x;
    }
};

class Ppu {
   private:
    std::array<byte, 32> palette_table{};
    std::array<byte, 2048> vram{};
    std::array<byte, 256> oam{};

    byte io_data_bus{};

    byte ppuctrl{};
    byte ppumask{};
    byte ppustatus{0x1F};
    byte oamaddr{};
    PpuAddrLatch ppuaddr{};
    std::optional<byte> ppudata_buf;  // PPUDATA read buffer
    PpuScrollLatch ppuscroll{};

    std::shared_ptr<Cartridge> cartridge{};
    std::shared_ptr<bool> nmi_requested{};

    // Properties for the PPU
    word BaseNametableAddress() const {
        switch (ppuctrl & 0b11) {
            case 0:
                return 0x2000;
            case 1:
                return 0x2400;
            case 2:
                return 0x2800;
            case 3:
                return 0x2C00;
        }

        return 0x2000;  // Shouldn't be possible
    }

    byte VramAddressIncrement() const { return (ppuctrl & 0x04) != 0x00 ? 32 : 1; }
    word SpritePatternTableAddress() const { return (ppuctrl & 0x08) != 0x00 ? 0x1000 : 0x0000; }
    word BgPatternTableAddress() const { return (ppuctrl & 0x10) != 0x00 ? 0x1000 : 0x0000; }
    byte SpriteHeight() const { return (ppuctrl & 0x20) != 0x00 ? 16 : 8; }
    bool NmiAtVBlank() const { return (ppuctrl & 0x80) != 0x00; }

    bool Grayscale() const { return (ppumask & 0x01) != 0x00; }
    bool ShowBackgroundInLeft() const { return (ppumask & 0x02) != 0x00; }
    bool ShowSpritesInLeft() const { return (ppumask & 0x04) != 0x00; }
    bool ShowBackground() const { return (ppumask & 0x08) != 0x00; }
    bool ShowSprites() const { return (ppumask & 0x10) != 0x00; }
    bool EmphasizeRed() const { return (ppumask & 0x20) != 0x00; }
    bool EmphasizeGreen() const { return (ppumask & 0x40) != 0x00; }
    bool EmphasizeBlue() const { return (ppumask & 0x80) != 0x00; }

    // TODO: Unstub me
    bool InVblank() const { return false; }

   public:
    Ppu() = default;

    Ppu(std::shared_ptr<Cartridge> cartridge, std::shared_ptr<bool> nmi_requested)
        : cartridge{std::move(cartridge)}, nmi_requested{std::move(nmi_requested)} {}

    void Tick() {}

    byte CpuRead(word address);
    void CpuWrite(word address, byte data);

    byte PpuRead(word address);
    void PpuWrite(word address, byte data);
};