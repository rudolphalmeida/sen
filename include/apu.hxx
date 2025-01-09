//
// Created by rudolph on 30/11/24.
//

#pragma once

#include <spdlog/spdlog.h>

#include "constants.hxx"
#include "cqueue.hpp"

constexpr byte DUTY_CYCLES[4] = {
    0b10000000,
    0b11000000,
    0b11110000,
    0b00111111,
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
    byte GetSample() const {
        if (timer < 8) {
            return 0x00;
        }

        const auto duty = (duty_cycle & (1 << duty_counter_bit)) >> duty_counter_bit;
        return duty * (constant_volume ? volume_reload : envelope_decay_level);
    }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0: UpdateVolume(data); break;
            case 1: UpdateSweep(data); break;
            case 2: UpdateTimerLow(data); break;
            case 3: UpdateTimerHigh(data); break;
            default: break;
        }
    }

    void ClockTimer() {
        if (timer == 0x00) {
            timer = timer_reload;
            duty_counter_bit = (duty_counter_bit - 1) & 0x07;
        } else {
            timer--;
        }
    }

    void ClockEnvelope() {
        if (!envelope_start) {
            if (envelope_divider == 0x00) {
                envelope_divider = volume_reload;
                if (envelope_decay_level == 0x00 && length_counter_halt) {
                    envelope_decay_level = 15;
                } else if (envelope_decay_level != 0x00) {
                    envelope_decay_level--;
                }
            }
        } else {
            envelope_start = false;
            envelope_decay_level = 15;
            envelope_divider = volume_reload;
        }
    }

private:
    word timer{}, timer_reload{};
    byte length_counter_load{};

    byte duty_counter_bit{0x00};

    byte duty_cycle{};
    bool length_counter_halt{false};
    bool constant_volume{false};
    byte volume_reload{};

    bool envelope_start{false};
    byte envelope_decay_level{};
    byte envelope_divider{};

    byte volume{};

    void UpdateVolume(const byte volume) {
        volume_reload = volume & 0x0F;
        constant_volume = (volume & 0x10) != 0x00;
        length_counter_halt = (volume & 0x20) != 0x00;
        duty_cycle = DUTY_CYCLES[(volume & 0xC0) >> 6];
    }

    void UpdateSweep(const byte sweep) const {
        spdlog::debug("TODO: APU Pulse Sweep");
    }

    void UpdateTimerLow(const byte timer_low) {
        timer_reload = (timer_reload & 0xFF00) | timer_low;
    }

    void UpdateTimerHigh(const byte timer_high) {
        timer_reload = (timer_reload & ~0x0700) | (static_cast<word>(timer_high & 0x07) << 8);
        envelope_start = true;
        duty_counter_bit = 0x00;
        // TODO: Writing to $4003/$4007 reloads the length counter, restarts the envelope, and resets the phase of the pulse generator.
    }
};

enum class ApuChannel: byte {
    Pulse1 = (1 << 0),
    Pulse2 = (1 << 1),
    Triangle = (1 << 2),
    Noise = (1 << 3),
    Dmc = (1 << 4),
};

struct OutputSamples {
    std::mutex samples_mutex{};
    gto::cqueue<float> samples;
    std::atomic_int num_samples{};
};

class Apu {
public:
    OutputSamples samples;

    explicit Apu(InterruptRequestFlag irq_requested): irq_requested(std::move(irq_requested)) {}

    void Tick(uint64_t cpu_cycles);

    [[maybe_unused]] static void Reset() { spdlog::error("Reset not implemented for APU"); }

    [[nodiscard]] static byte CpuRead(word address);
    void CpuWrite(word address, byte data);

    friend class Debugger;

private:
    ApuPulse pulse_1{}, pulse_2{};

    InterruptRequestFlag irq_requested{};

    uint64_t frame_begin_cpu_cycle{0x00};
    FrameCounterStepMode step_mode{FrameCounterStepMode::FourStep};
    bool raise_irq{false};

    byte enabled_channels{0x00};

    void UpdateFrameCounter(byte data);
    static float Mix(const byte pulse1_sample, const byte pulse2_sample);
};
