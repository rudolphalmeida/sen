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

    void tick() { cycles++; }

    byte cpu_read(const word address) const { return ram[address]; }

    void cpu_write(const word address, const byte data) { ram[address] = data; }

    byte ticked_cpu_read(const word address) {
        auto expected_cycle = expected_cycles[cycles];

        REQUIRE(expected_cycle.verb == "read");
        REQUIRE(expected_cycle.address == address);

        tick();
        const auto data = cpu_read(address);
        REQUIRE(expected_cycle.data == data);

        return data;
    }

    void ticked_cpu_write(const word address, const byte data) {
        auto expected_cycle = expected_cycles[cycles];

        REQUIRE(expected_cycle.verb == "write");
        REQUIRE(expected_cycle.address == address);
        REQUIRE(expected_cycle.data == data);

        tick();
        cpu_write(address, data);
    }
};
