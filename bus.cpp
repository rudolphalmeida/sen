#include "bus.hxx"
#include "util.hxx"

byte Bus::UntickedCpuRead(word address) {
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        return internal_ram[address % 0x800];
    } else if (InRange<word>(0x2000, address, 0x3FFF)) {
        return ppu->CpuRead(address);
    } else if (InRange<word>(0x4000, address, 0x4015)) {
        return 0xFF;
    } else if (InRange<word>(0x4016, address, 0x4017)) {
        return controller->CpuRead(address);
    } else if (InRange<word>(0x4018, address, 0x401F)) {
        return 0xFF;
    } else {
        return cartridge->CpuRead(address);
    }
}

void Bus::UntickedCpuWrite(word address, byte data) {
    if (InRange<word>(0x0000, address, 0x1FFF)) {
        internal_ram[address % 0x800] = data;
    } else if (InRange<word>(0x2000, address, 0x3FFF)) {
        ppu->CpuWrite(address, data);
    } else if (address == 0x4014) {
        PerformOamDma(data);
    } else if (InRange<word>(0x4000, address, 0x4015)) {
    } else if (InRange<word>(0x4016, address, 0x4017)) {
        controller->CpuWrite(address, data);
    } else if (InRange<word>(0x4018, address, 0x401F)) {
    } else {
        cartridge->CpuWrite(address, data);
    }
}

void Bus::PerformOamDma(byte high) { // - 513 ticks (ignoring +1 for put cycle)
    word address = static_cast<word>(high) << 8;
    word end = address + 0x100;

    // Wait cycle - 1
    Tick();
    for (; address < end; address++) {
        CpuWrite(0x2004, CpuRead(address));  // 2 ticks - total 256
    }
}
