#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "cartridge.hxx"
#include "constants.hxx"

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};

Cartridge ParseRomFile(const RomArgs& rom_args);

class Sen {
   public:
    Sen(RomArgs rom_args);
};
