#include <array>
#include <fstream>
#include <memory>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#define CPU_TEST

#include "../bus.hxx"
#include "../constants.hxx"
#include "../cpu.hxx"
#include "flatbus.hxx"

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

#define OPCODE_TEST(opc)                                                         \
    TEST_CASE("Test opcode " #opc, "[opcodeTest" #opc "]") {                     \
        std::ifstream tests_json_file("./ProcessorTests/6502/v1/" #opc ".json"); \
        auto tests_data = nlohmann::json::parse(tests_json_file);                \
        TestOpcode(tests_data);                                                  \
    }

OPCODE_TEST(01)
OPCODE_TEST(02)
