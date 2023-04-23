#include <spdlog/spdlog.h>

#include "bus.hxx"
#include "util.hxx"

byte Bus::UntickedCpuRead(word address) {
    if (inRange<word>(0x0000, address, 0x1FFF)) {
        return internal_ram[address % 0x800];
    } else if (inRange<word>(0x2000, address, 0x3FFF)) {
        return ppu->CpuRead(address);
    } else if (inRange<word>(0x4000, address, 0x4017)) {
        return 0xFF;
    } else if (inRange<word>(0x4018, address, 0x401F)) {
        return 0xFF;
    } else {
        return cartridge->CpuRead(address);
    }
}

void Bus::UntickedCpuWrite(word address, byte data) {
    if (inRange<word>(0x0000, address, 0x1FFF)) {
        internal_ram[address % 0x800] = data;
    } else if (inRange<word>(0x2000, address, 0x3FFF)) {
        ppu->CpuWrite(address, data);
    } else if (inRange<word>(0x4000, address, 0x4017)) {
    } else if (inRange<word>(0x4018, address, 0x401F)) {
    } else {
        cartridge->CpuWrite(address, data);
    }
}
