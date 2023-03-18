#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "bus.hxx"
#include "cartridge.hxx"
#include "constants.hxx"
#include "cpu.hxx"

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};

Cartridge ParseRomFile(const RomArgs& rom_args);

class Sen {
   private:
    std::shared_ptr<Bus> bus;
    Cpu cpu;

   public:
    Sen(RomArgs rom_args);
};
