//
// Created by rudolph on 30/11/24.
//

#pragma once

#include <spdlog/spdlog.h>

#include "constants.hxx"

constexpr byte DUTY_CYCLES[4] = {
    0b01000000,
    0b01100000,
    0b01111000,
    0b10011111,
};

constexpr byte LENGTH_COUNTER_LOADS[] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

enum class FrameCounterStepMode {
    FourStep,
    FiveStep,
};

class ApuPulse {
public:
    byte Tick() { return 0x00; }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0: UpdateVolume(data); break;
            case 1: UpdateSweep(data); break;
            case 2: UpdateTimerLow(data); break;
            case 3: UpdateTimerHigh(data); break;
            default: break;
        }
    }

private:
    byte duty_cycle{};
    bool length_counter_halt{false};
    bool constant_volume{false};
    byte volume_envelope{};

    word timer{};
    byte length_counter_load{};

    void UpdateVolume(const byte volume) {
        volume_envelope = volume & 0x0F;
        constant_volume = (volume & 0x10) != 0x00;
        length_counter_halt = (volume & 0x20) != 0x00;
        duty_cycle = DUTY_CYCLES[(volume & 0xC0) >> 6];
    }

    void UpdateSweep(const byte sweep) const {
        spdlog::debug("TODO: APU Pulse Sweep");
    }

    void UpdateTimerLow(const byte timer_low) {
        timer = (timer & 0xFF00) | timer_low;
    }

    void UpdateTimerHigh(const byte timer_high) {
        timer = (timer & ~0x0700) | (static_cast<word>(timer_high & 0x07) << 8);
        // TODO: Writing to $4003/$4007 reloads the length counter, restarts the envelope, and resets the phase of the pulse generator.
    }
};

class ApuMixer {
public:
    byte Mix(const byte pulse1_sample, const byte pulse2_sample) { return 0x00; }
};

class Apu {
public:
    explicit Apu(InterruptRequestFlag irq_requested): irq_requested(std::move(irq_requested)) {}

    void Tick(uint64_t cpu_cycles);

    [[nodiscard]] static byte CpuRead(word address);
    void CpuWrite(word address, byte data);

private:
    ApuPulse pulse_1{}, pulse_2{};
    ApuMixer mixer{};

    InterruptRequestFlag irq_requested{};

    uint64_t frame_begin_cpu_cycle{0x00};
    FrameCounterStepMode step_mode{FrameCounterStepMode::FourStep};
    bool raise_irq{false};

    void UpdateFrameCounter(byte data);
};
