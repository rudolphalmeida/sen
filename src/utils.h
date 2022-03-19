//
// Bitwise and small utility functions
// Created by Rudolph on 19/3/22.
//

#ifndef SEN_SRC_UTILS_H_
#define SEN_SRC_UTILS_H_

#include <cstdint>

using byte = uint8_t;
using word = uint16_t;
using sbyte = int8_t;

template <typename T>
bool isBitSet(T value, byte index) {
    return (value & (0b1 << index)) != 0;
}

template <typename T>
T setBit(T value, byte index) {
    return value | (0b1 << index);
}

template <typename T>
T resetBit(T value, byte index) {
    return value & ~(0b1 << index);
}

#endif  // SEN_SRC_UTILS_H_
