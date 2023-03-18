#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "cartridge.hxx"
#include "constants.hxx"

struct RomArgs {
    std::vector<byte> rom;
    std::optional<std::vector<byte>> ram = std::nullopt;
};

std::shared_ptr<Cartridge> ParseRomFile(const RomArgs& rom_args);

class Sen {
   private:
    std::shared_ptr<Cartridge> cartridge;

   public:
    Sen(RomArgs rom_args);
};
