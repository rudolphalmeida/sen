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
#include "events.hxx"
#include "ppu.hxx"
// Stay down!
#include "cpu.hxx"

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};

std::shared_ptr<Cartridge> ParseRomFile(const RomArgs& rom_args);

class Sen {
  private:
    EventBus event_bus{};
    Cpu<Bus> cpu;
    std::shared_ptr<Bus> bus;
    std::shared_ptr<Ppu> ppu;
    std::shared_ptr<Controller> controller;
    std::shared_ptr<Apu> apu;

    uint64_t carry_over_cycles{};

    InterruptRequestFlag nmi_requested, irq_requested;
    bool running{false};

  public:
    explicit Sen(const RomArgs& rom_args, const std::shared_ptr<AudioQueue>& sink);

    template<typename E>
    void register_handler(Handler* handler) {
        event_bus.register_handler<E>(handler);
    }

    [[nodiscard]] uint64_t FrameCount() const {
        return ppu->frame_count;
    }

    void RunForCycles(uint64_t cycles);
    void StepOpcode();
    void RunForOneScanline();
    void RunForOneFrame();

    void set_pressed_keys(ControllerPort port, byte key) const;

    friend class Debugger;
};
