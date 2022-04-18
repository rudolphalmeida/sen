//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MEMORY_H_
#define SEN_SRC_MEMORY_H_

#include "utils.h"

// Abstract Class for Memory space on the CPU bus
class CpuAddressSpace {
   public:
    virtual byte Read(word address) = 0;
    virtual void Write(word address, byte data) = 0;
};

#endif  // SEN_SRC_MEMORY_H_
