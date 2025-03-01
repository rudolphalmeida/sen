#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "apu.hxx"
#include "bus.hxx"
#include "cartridge.hxx"
#include "constants.hxx"
#include "controller.hxx"
#include "cpu.hxx"
#include "ppu.hxx"

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};

std::shared_ptr<Cartridge> ParseRomFile(const RomArgs& rom_args);

class Sen {
  private:
    uint64_t carry_over_cycles{};
    bool running{false};

    std::shared_ptr<Bus> bus;
    std::shared_ptr<Ppu> ppu;
    std::shared_ptr<Controller> controller;
    std::shared_ptr<Apu> apu;
    Cpu<Bus> cpu;

    InterruptRequestFlag nmi_requested{}, irq_requested{};

  public:
    explicit Sen(const RomArgs& rom_args, const std::shared_ptr<AudioQueue>& sink);

    uint64_t FrameCount() const {
        return ppu->frame_count;
    }

    void RunForCycles(uint64_t cycles);
    void StepOpcode();
    void RunForOneScanline();
    void RunForOneFrame();

    void set_pressed_keys(ControllerPort port, byte key) const;

    friend class Debugger;
};
