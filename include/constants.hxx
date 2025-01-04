#pragma once

#include <cstdint>

#define NES_WIDTH 256
#define NES_HEIGHT 240

using byte = uint8_t;
using word = uint16_t;
using InterruptRequestFlag = std::shared_ptr<bool>;
