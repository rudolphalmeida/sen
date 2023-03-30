#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "bus.hxx"
#include "cartridge.hxx"
#include "constants.hxx"
#include "cpu.hxx"
#include "ppu.hxx"

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};

std::shared_ptr<Cartridge> ParseRomFile(const RomArgs& rom_args);

class Sen {
   private:
    std::shared_ptr<Bus> bus;
    std::shared_ptr<Ppu> ppu;
    Cpu cpu;

    std::shared_ptr<bool> nmi_requested{};

   public:
    Sen(RomArgs rom_args);

    void Run();
};
