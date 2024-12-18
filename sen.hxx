#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

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
    Cpu<Bus> cpu;

    std::shared_ptr<bool> nmi_requested{};

    static constexpr uint64_t CYCLES_PER_FRAME{29780};

   public:
    explicit Sen(const RomArgs& rom_args);

    void StepOpcode();
    void RunForOneScanline();
    void RunForOneFrame();

    void ControllerPress(ControllerPort port, ControllerKey key) const;
    void ControllerRelease(ControllerPort port, ControllerKey key) const;

    friend class Debugger;
};
