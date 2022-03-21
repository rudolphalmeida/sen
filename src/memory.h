//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_MEMORY_H_
#define SEN_SRC_MEMORY_H_

#include "utils.h"

// Abstract Class for Memory-Mapped Components
class Memory {
   public:
    [[nodiscard]] virtual byte Read(word address) const = 0;
    virtual void Write(word address, byte data) = 0;
};

#endif  // SEN_SRC_MEMORY_H_
