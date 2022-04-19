//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MEMORY_H_
#define SEN_SRC_MEMORY_H_

#include "utils.h"

// Abstract Class for Memory space on the CPU bus
class CpuBus {
   public:
    virtual byte CpuRead(word address) = 0;
    virtual void CpuWrite(word address, byte data) = 0;
};

// Abstract Class for Memory space on the PPU bus
class PpuBus {
   public:
    virtual byte PpuRead(word address) = 0;
    virtual void PpuWrite(word address, byte data) = 0;
};

#endif  // SEN_SRC_MEMORY_H_
