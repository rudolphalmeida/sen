#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "util.hxx"

byte Bus::UntickedCpuRead(word address) {
    if (inRange<word>(0x0000, address, 0x1FFF)) {
        return internal_ram[address % 0x800];
    } else if (inRange<word>(0x2000, address, 0x3FFF)) {
        return ppu->CpuRead(address);
    } else if (inRange<word>(0x4000, address, 0x4017)) {
        spdlog::debug("Read from not implemented IO address {:#06X}", address);
        return 0xFF;
    } else if (inRange<word>(0x4018, address, 0x401F)) {
        spdlog::debug("Read from not implemented CPU Test Mode {:#06X}", address);
        return 0xFF;
    } else {
        return cartridge.CpuRead(address);
    }
}

void Bus::UntickedCpuWrite(word address, byte data) {
    if (inRange<word>(0x0000, address, 0x1FFF)) {
        internal_ram[address % 0x800] = data;
    } else if (inRange<word>(0x2000, address, 0x3FFF)) {
        ppu->CpuWrite(address, data);
    } else if (inRange<word>(0x4000, address, 0x4017)) {
        spdlog::debug("Write to not implemented IO address {:#06X} with {:#04X}", address, data);
    } else if (inRange<word>(0x4018, address, 0x401F)) {
        spdlog::debug("Write to not implemented CPU Test Mode {:#06X} with {:#04X}", address, data);
    } else {
        cartridge.CpuWrite(address, data);
    }
}
