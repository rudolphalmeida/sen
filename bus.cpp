#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "util.hxx"

byte Bus::CpuRead(word addr) {
    if (inRange<word>(0x0000, addr, 0x1FFF)) {
        return internal_ram[addr % 0x800];
    } else if (inRange<word>(0x2000, addr, 0x3FFF)) {
        spdlog::debug("Read from not implented PPU address {:#06X}", addr);
        return 0xFF;
    } else if (inRange<word>(0x4000, addr, 0x4017)) {
        spdlog::debug("Read from not implemented IO address {:#06X}", addr);
        return 0xFF;
    } else if (inRange<word>(0x4018, addr, 0x401F)) {
        spdlog::debug("Read from not implemented CPU Test Mode {:#06X}", addr);
        return 0xFF;
    } else {
        return cartridge.CpuRead(addr);
    }
}

void Bus::CpuWrite(word addr, byte data) {
    if (inRange<word>(0x0000, addr, 0x1FFF)) {
        internal_ram[addr % 0x800] = data;
    } else if (inRange<word>(0x2000, addr, 0x3FFF)) {
        spdlog::debug("Write to not implented PPU address {:#06X} with {:#04X}", addr, data);
    } else if (inRange<word>(0x4000, addr, 0x4017)) {
        spdlog::debug("Write to not implemented IO address {:#06X} with {:#04X}", addr, data);
    } else if (inRange<word>(0x4018, addr, 0x401F)) {
        spdlog::debug("Write to not implemented CPU Test Mode {:#06X} with {:#04X}", addr, data);
    } else {
        cartridge.CpuWrite(addr, data);
    }
}
