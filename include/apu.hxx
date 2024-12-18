//
// Created by rudolph on 30/11/24.
//

#pragma once

#include "constants.hxx"

class Apu {
public:
    void Tick();

    [[nodiscard]] byte CpuRead(word address) const;
    void CpuWrite(word address, byte data);

private:
    // Registers
    byte sq1_vol{}, sq1_sweep{}, sq1_lo{}, sq1_hi{};
    byte sq2_vol{}, sq2_sweep{}, sq2_lo{}, sq2_hi{};
    byte tri_linear{}, tri_lo{}, tri_hi{};
    byte noise_vol{}, noise_lo{}, noise_hi{};
    byte dmc_freq{}, dmc_raw{}, dmc_start{}, dmc_len{};
    byte snd_chn{};
    byte frame_counter{};
};
