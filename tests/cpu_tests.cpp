#include <array>
#include <fstream>
#include <memory>
#include <stdexcept>

#include <fmt/core.h>
#include <fmt/format.h>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#define CPU_TEST

#include "../bus.hxx"
#include "../constants.hxx"
#include "../cpu.hxx"
#include "flatbus.hxx"

nlohmann::json LoadTestCasesJsonFile(byte opcode) {
    std::ifstream tests_json_file(fmt::format("./ProcessorTests/nes6502/v1/{:02x}.json", opcode));
    return nlohmann::json::parse(tests_json_file);
}

void TestOpcode(nlohmann::json tests_data) {
    auto nmi_requested = std::make_shared<bool>(false);

    for (auto test_case : tests_data) {
        auto bus = std::make_shared<FlatBus>();
        REQUIRE(bus->cycles == 0);  // Tests require cycles to start at zero
        Cpu<FlatBus> cpu{bus, nmi_requested};

        auto initial_state = test_case["initial"];
        cpu.pc = initial_state["pc"];
        cpu.s = initial_state["s"];
        cpu.a = initial_state["a"];
        cpu.x = initial_state["x"];
        cpu.y = initial_state["y"];
        cpu.p = initial_state["p"];
        for (auto ram_state : initial_state["ram"]) {
            bus->UntickedCpuWrite(static_cast<word>(ram_state[0]), static_cast<byte>(ram_state[1]));
        }

        cpu.Execute();

        auto final_state = test_case["final"];
        REQUIRE(cpu.pc == final_state["pc"]);
        REQUIRE(cpu.s == final_state["s"]);
        REQUIRE(cpu.a == final_state["a"]);
        REQUIRE(cpu.x == final_state["x"]);
        REQUIRE(cpu.y == final_state["y"]);
        REQUIRE(cpu.p == final_state["p"]);
        for (auto ram_state : final_state["ram"]) {
            REQUIRE(bus->UntickedCpuRead(static_cast<word>(ram_state[0])) ==
                    static_cast<byte>(ram_state[1]));
        }

        REQUIRE(bus->cycles == test_case["cycles"].size());
    }
}

#define OPCODE_TEST(opc)                                     \
    TEST_CASE("Test opcode " #opc, "[opcodeTest" #opc "]") { \
        spdlog::cfg::load_env_levels();                      \
        TestOpcode(LoadTestCasesJsonFile(opc));              \
    }

// Only testing for the legal opcodes and JAMs

OPCODE_TEST(0x01)
OPCODE_TEST(0x02)
OPCODE_TEST(0x04)
OPCODE_TEST(0x05)
OPCODE_TEST(0x06)
OPCODE_TEST(0x08)
OPCODE_TEST(0x09)
OPCODE_TEST(0x0A)
OPCODE_TEST(0x0D)
OPCODE_TEST(0x0E)

OPCODE_TEST(0x10)
OPCODE_TEST(0x11)
OPCODE_TEST(0x15)
OPCODE_TEST(0x16)
OPCODE_TEST(0x18)
OPCODE_TEST(0x19)
OPCODE_TEST(0x1D)
OPCODE_TEST(0x1E)

OPCODE_TEST(0x20)
OPCODE_TEST(0x21)
OPCODE_TEST(0x24)
OPCODE_TEST(0x25)
OPCODE_TEST(0x26)
OPCODE_TEST(0x28)
OPCODE_TEST(0x29)
OPCODE_TEST(0x2A)
OPCODE_TEST(0x2C)
OPCODE_TEST(0x2D)
OPCODE_TEST(0x2E)

OPCODE_TEST(0x30)
OPCODE_TEST(0x31)
OPCODE_TEST(0x35)
OPCODE_TEST(0x36)
OPCODE_TEST(0x38)
OPCODE_TEST(0x39)
OPCODE_TEST(0x3D)
OPCODE_TEST(0x3E)

OPCODE_TEST(0x40)
OPCODE_TEST(0x41)
OPCODE_TEST(0x45)
OPCODE_TEST(0x46)
OPCODE_TEST(0x48)
OPCODE_TEST(0x49)
OPCODE_TEST(0x4A)
OPCODE_TEST(0x4C)
OPCODE_TEST(0x4D)
OPCODE_TEST(0x4E)

OPCODE_TEST(0x50)
OPCODE_TEST(0x51)
OPCODE_TEST(0x55)
OPCODE_TEST(0x56)
OPCODE_TEST(0x58)
OPCODE_TEST(0x59)
OPCODE_TEST(0x5D)
OPCODE_TEST(0x5E)

OPCODE_TEST(0x60)
OPCODE_TEST(0x61)
OPCODE_TEST(0x65)
OPCODE_TEST(0x66)
OPCODE_TEST(0x68)
OPCODE_TEST(0x69)
OPCODE_TEST(0x6A)
OPCODE_TEST(0x6C)
OPCODE_TEST(0x6D)
OPCODE_TEST(0x6E)

OPCODE_TEST(0x70)
OPCODE_TEST(0x71)
OPCODE_TEST(0x75)
OPCODE_TEST(0x76)
OPCODE_TEST(0x78)
OPCODE_TEST(0x79)
OPCODE_TEST(0x7D)
OPCODE_TEST(0x7E)

OPCODE_TEST(0x81)
OPCODE_TEST(0x84)
OPCODE_TEST(0x85)
OPCODE_TEST(0x86)
OPCODE_TEST(0x88)
OPCODE_TEST(0x8A)
OPCODE_TEST(0x8C)
OPCODE_TEST(0x8D)
OPCODE_TEST(0x8E)

OPCODE_TEST(0x90)
OPCODE_TEST(0x91)
OPCODE_TEST(0x94)
OPCODE_TEST(0x95)
OPCODE_TEST(0x96)
OPCODE_TEST(0x98)
OPCODE_TEST(0x99)
OPCODE_TEST(0x9A)
OPCODE_TEST(0x9D)

OPCODE_TEST(0xA0)
OPCODE_TEST(0xA1)
OPCODE_TEST(0xA2)
OPCODE_TEST(0xA4)
OPCODE_TEST(0xA5)
OPCODE_TEST(0xA6)
OPCODE_TEST(0xA8)
OPCODE_TEST(0xA9)
OPCODE_TEST(0xAA)
OPCODE_TEST(0xAC)
OPCODE_TEST(0xAD)
OPCODE_TEST(0xAE)

OPCODE_TEST(0xB0)
OPCODE_TEST(0xB1)
OPCODE_TEST(0xB4)
OPCODE_TEST(0xB5)
OPCODE_TEST(0xB6)
OPCODE_TEST(0xB8)
OPCODE_TEST(0xB9)
OPCODE_TEST(0xBA)
OPCODE_TEST(0xBC)
OPCODE_TEST(0xBD)
OPCODE_TEST(0xBE)

OPCODE_TEST(0xC0)
OPCODE_TEST(0xC1)
OPCODE_TEST(0xC4)
OPCODE_TEST(0xC5)
OPCODE_TEST(0xC6)
OPCODE_TEST(0xC8)
OPCODE_TEST(0xC9)
OPCODE_TEST(0xCA)
OPCODE_TEST(0xCC)
OPCODE_TEST(0xCD)
OPCODE_TEST(0xCE)

OPCODE_TEST(0xD0)
OPCODE_TEST(0xD1)
OPCODE_TEST(0xD5)
OPCODE_TEST(0xD6)
OPCODE_TEST(0xD8)
OPCODE_TEST(0xD9)
OPCODE_TEST(0xDD)
OPCODE_TEST(0xDE)

OPCODE_TEST(0xE0)
OPCODE_TEST(0xE1)
OPCODE_TEST(0xE4)
OPCODE_TEST(0xE5)
OPCODE_TEST(0xE6)
OPCODE_TEST(0xE8)
OPCODE_TEST(0xE9)
OPCODE_TEST(0xEA)
OPCODE_TEST(0xEC)
OPCODE_TEST(0xED)
OPCODE_TEST(0xEE)

OPCODE_TEST(0xF0)
OPCODE_TEST(0xF1)
OPCODE_TEST(0xF5)
OPCODE_TEST(0xF6)
OPCODE_TEST(0xF8)
OPCODE_TEST(0xF9)
OPCODE_TEST(0xFD)
OPCODE_TEST(0xFE)
