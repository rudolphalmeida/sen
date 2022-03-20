//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_ROM_H_
#define SEN_SRC_ROM_H_

#include <filesystem>

#include "utils.h"

enum class RomMirroring { Horizontal = 0, Vertical = 1, FourScreenVram = 2 };

class Rom {
   public:
    static Rom ParseRomFile(const std::filesystem::path& romFilePath);

    [[nodiscard]] RomMirroring Mirroring() const {
        if (isBitSet(flag6, 3)) {
            return RomMirroring::FourScreenVram;
        }

        return isBitSet(flag6, 0) ? RomMirroring::Vertical
                                  : RomMirroring::Horizontal;
    }

    [[nodiscard]] bool HasBatteryBackedRam() const {
        return isBitSet(flag6, 1);
    }

    [[nodiscard]] bool HasTrainer() const { return isBitSet(flag6, 2); }

    [[nodiscard]] byte MapperNumber() const {
        return (flag7 & 0xF0) | (flag6 & 0xF0);
    }

   private:
    explicit Rom(byte flag6,
                 byte flag7,
                 std::vector<byte>&& trainer,
                 std::vector<byte>&& prgRom,
                 std::vector<byte>&& chrRom)
        : flag6{flag6},
          flag7{flag7},
          trainer{std::move(trainer)},
          prgRom{std::move(prgRom)},
          chrRom{std::move(chrRom)} {}

    byte flag6, flag7;

    std::vector<byte> trainer;
    std::vector<byte> prgRom;
    std::vector<byte> chrRom;
};

#endif  // SEN_SRC_ROM_H_
