//
// Created by rudolph on 20/3/22.
//

#ifndef SEN_SRC_CARTRIDGE_H_
#define SEN_SRC_CARTRIDGE_H_

#include "memory.h"
#include "rom.h"
#include "utils.h"

/*
 * Abstract Base Class for Mappers
 * */
class Mapper {
   public:
    [[nodiscard]] virtual byte Read(const Rom& rom, word address) const = 0;
    virtual void Write(Rom& rom, word address, byte data) = 0;

    virtual ~Mapper() = default;
};

// Contains the cartridge data and the mapper implementation
class Cartridge : public CpuBus {
   public:
    explicit Cartridge(Rom&& rom);

    [[nodiscard]] byte CpuRead(word address) override;
    void CpuWrite(word address, byte data) override;

   private:
    explicit Cartridge(Rom&& rom, std::unique_ptr<Mapper> mapper)
        : rom{rom}, mapper{std::move(mapper)} {}

    Rom rom;
    std::unique_ptr<Mapper> mapper;
};

// Mapper Declarations - Names are based on the iNES mapper number

// Mapper 0 - NROM
class Mapper000 : public Mapper {
    static const byte NUMBER = 0x00;

    [[nodiscard]] byte Read(const Rom& rom, word address) const override;

    void Write(Rom& rom, word address, byte data) override;
};

#endif  // SEN_SRC_CARTRIDGE_H_
