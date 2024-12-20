#pragma once

#include <array>
#include <vector>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "constants.hxx"

struct Cycle {
    std::string verb{"read"};
    word address{};
    byte data{};
};

class FlatBus {
   private:
    std::array<byte, 65536> ram{};
    const std::vector<Cycle>& expected_cycles;

   public:
    unsigned int cycles{};

    explicit FlatBus(const std::vector<Cycle>& expected_cycles): expected_cycles{expected_cycles} {};

    void Tick() { cycles++; }

    byte UntickedCpuRead(word address) { return ram[address]; }

    void UntickedCpuWrite(word address, byte data) { ram[address] = data; }

    byte CpuRead(word address) {
        auto expected_cycle = expected_cycles[cycles];

        REQUIRE(expected_cycle.verb == "read");
        REQUIRE(expected_cycle.address == address);

        Tick();
        auto data = UntickedCpuRead(address);
        REQUIRE(expected_cycle.data == data);

        return data;
    }

    void CpuWrite(word address, byte data) {
        auto expected_cycle = expected_cycles[cycles];

        REQUIRE(expected_cycle.verb == "write");
        REQUIRE(expected_cycle.address == address);
        REQUIRE(expected_cycle.data == data);

        Tick();
        UntickedCpuWrite(address, data);
    }
};
