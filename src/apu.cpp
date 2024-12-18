//
// Created by rudolph on 30/11/24.
//

#include "apu.hxx"

void Apu::Tick() {}

byte Apu::CpuRead(const word address) const {
    if (address == 0x4015) {
        return snd_chn;
    }
    // TODO: Open bus
    return 0xFF;
}

void Apu::CpuWrite(const word address, const byte data) {
    switch (address) {
        case 0x4000:
            sq1_vol = data;
            break;
        case 0x4001:
            sq1_sweep = data;
            break;
        case 0x4002:
            sq1_lo = data;
            break;
        case 0x4003:
            sq1_hi = data;
            break;
        case 0x4004:
            sq2_vol = data;
            break;
        case 0x4005:
            sq2_sweep = data;
            break;
        case 0x4006:
            sq2_lo = data;
            break;
        case 0x4007:
            sq2_hi = data;
            break;
        case 0x4008:
            tri_linear = data;
            break;
        case 0x400A:
            tri_lo = data;
            break;
        case 0x400B:
            tri_hi = data;
            break;
        case 0x400C:
            noise_vol = data;
            break;
        case 0x400E:
            noise_lo = data;
            break;
        case 0x400F:
            noise_hi = data;
            break;
        case 0x4010:
            dmc_freq = data;
            break;
        case 0x4011:
            dmc_raw = data;
            break;
        case 0x4012:
            dmc_start = data;
            break;
        case 0x4013:
            dmc_len = data;
            break;
        case 0x4015:
            snd_chn = data;
            break;
        case 0x4017:
            frame_counter = data;
            break;
        default: {
        };
    }
}