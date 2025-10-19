#pragma once

#include <cstdint>
#include <memory>

constexpr int NES_WIDTH = 256;
constexpr int NES_HEIGHT = 240;

using byte = uint8_t;
using word = uint16_t;
using InterruptRequestFlag = std::shared_ptr<bool>;

// Clocks and timings
constexpr uint64_t NTSC_NES_CLOCK_FREQ{1789773};
constexpr uint64_t CYCLES_PER_FRAME{29780};

// Memory sizes
constexpr size_t IWRAM_SIZE{0x800};