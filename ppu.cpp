#include <spdlog/spdlog.h>

#include "ppu.hxx"

byte Ppu::CpuRead(word address) {
    switch (0x2000 + (address & 0b111)) {
        case 0x2002:
            io_data_bus = (ppustatus & 0xE0) | (io_data_bus & 0x1F);
            ppustatus &= 0x7F;  // Reading this register clears bit 7
            ppuaddr.ResetLatch();
            ppuscroll.ResetLatch();
            break;
        case 0x2004:
            io_data_bus = oam[oamaddr];
            break;
        case 0x2007:
            auto ppu_address = static_cast<word>(ppuaddr);
            if (ppu_address > 0x3EFF) {  // Reading palettes
                io_data_bus = PpuRead(ppu_address);
            } else if (ppudata_buf) {
                // Use previously read value from buffer and reset buffer
                io_data_bus = ppudata_buf.value();
                ppudata_buf.reset();
            } else {
                // Read value into PPUDATA read buffer
                ppudata_buf.emplace(PpuRead(ppu_address));
            }
            ppuaddr.IncrementBy(VramAddressIncrement());
            break;
    }

    return io_data_bus;
}

void Ppu::CpuWrite(word address, byte data) {
    io_data_bus = data;
    switch (0x2000 + (address & 0b111)) {
        case 0x2000:
            if (InVblank()                       // If the PPU is in Vblank
                && ((ppustatus & 0x80) != 0x00)  // and the PPUSTATUS vblank flag is still set
                && ((ppuctrl & 0x80) != 0x00)    // and changing the NMI flag in PPUCTRL from 0
                && ((data & 0x80) == 0x80))      // to 1
            {
                *nmi_requested = true;  // will immediately generate an NMI
            }
            ppuctrl = data;
            break;
        case 0x2001:
            ppumask = data;
            break;
        case 0x2003:
            oamaddr = data;
            break;
        case 0x2004:
            // TODO: Ignore these writes if during rendering
            oam[oamaddr++] = data;
            break;
        case 0x2005:
            ppuscroll.WriteByte(data);
            break;
        case 0x2006:
            ppuaddr.WriteByte(data);
            break;
        case 0x2007:
            PpuWrite(static_cast<word>(ppuaddr), data);
            ppuaddr.IncrementBy(VramAddressIncrement());
            break;
        default:
            spdlog::debug("Write to not implented PPU address {:#06X} with {:#04X}", address, data);
    }
}

byte Ppu::PpuRead(word) {
    return 0xFF;
}

void Ppu::PpuWrite(word, byte) {}
