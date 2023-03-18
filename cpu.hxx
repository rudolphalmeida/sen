#pragma once

#include <cassert>
#include <memory>

#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "constants.hxx"

enum StatusFlag : byte {
    Carry = (1 << 0),
    Zero = (1 << 1),
    InterruptDisable = (1 << 2),
    _Decimal = (1 << 3),  // Unused in NES
    _B = 0x30,            // No CPU effect, bits 45
    Overflow = (1 << 6),
    Negative = (1 << 7),
};

class Cpu {
   private:
    byte a{};       // Accumalator
    byte x{}, y{};  // General purpose registers
    word pc{};      // Program counter
    byte s{};       // Stack pointer
    byte p{};       // Status register

    std::shared_ptr<Bus> bus{};

   public:
    Cpu() = default;

    Cpu(std::shared_ptr<Bus> bus) : bus{std::move(bus)} { spdlog::debug("Initialized CPU"); }

    bool IsSet(StatusFlag flag) const {
        // These flags are unused in the NES
        assert(flag != StatusFlag::_Decimal || flag != StatusFlag::_B);

        return (p & static_cast<byte>(flag)) != 0;
    }

    void UpdateStatusFlag(StatusFlag flag, bool value) {
        // These flags are unused in the NES
        assert(flag != StatusFlag::_Decimal || flag != StatusFlag::_B);

        if (value) {
            p |= static_cast<byte>(flag);
        } else {
            p &= ~static_cast<byte>(flag);
        }
    }

    // Runs the CPU startup procedure. Should run for 7 NES cycles
    void Start(){};

    void Execute(){};
};
