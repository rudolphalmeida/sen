#pragma once

#include <cstdint>
#include <memory>

#define NES_WIDTH 256
#define NES_HEIGHT 240

// In Hz
constexpr uint64_t NTSC_NES_CLOCK_FREQ{1789773};
constexpr uint64_t CYCLES_PER_FRAME{29780};

using byte = uint8_t;
using word = uint16_t;
using InterruptRequestFlag = std::shared_ptr<bool>;
