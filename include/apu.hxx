//
// Created by rudolph on 30/11/24.
//

#pragma once

#include <spdlog/spdlog.h>

#include "constants.hxx"

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

class AudioQueue {
   public:
    virtual ~AudioQueue() = default;

    virtual void PushSample(float sample) = 0;
    virtual void GetSamples(uint8_t * output, size_t samples) = 0;
    virtual void Clear() = 0;
};

class ApuPulse {
public:
    byte length_counter{0x00};
    bool enabled{false};

    explicit ApuPulse(const bool use_twos_complement): use_twos_complement{use_twos_complement} {}

    byte GetSample() const {
        if (timer < 8 || length_counter == 0x00 || target_period > 0x7FF) {
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

    void ClockLengthCounter() {
        if (!length_counter_halt && length_counter != 0x00) {
            length_counter--;
        }
    }

    void ClockSweep() {
        UpdateTargetPeriod();

        if (sweep_counter == 0x00 && sweep_enabled && !(timer < 8 || target_period > 0x7FF)) {
            timer_reload = target_period;
        }

        if (sweep_counter == 0x00 || sweep_reload) {
            sweep_counter = sweep_divider_load;
            sweep_reload = false;

            if (sweep_enabled) {
                timer_reload = target_period;
            }
        } else {
            sweep_counter--;
        }
    }

    void LoadLengthCounter(const byte value) {
        length_counter = value;
    }

    void LoadLengthCounter() {
        LoadLengthCounter(LENGTH_COUNTER_LOADS[length_counter_load]);
    }

private:
    word timer{}, timer_reload{}, target_period{};
    byte length_counter_load{};

    byte duty_counter_bit{0x00};

    byte duty_cycle{};
    bool length_counter_halt{false};
    bool constant_volume{false};
    byte volume_reload{};

    bool envelope_start{false};
    byte envelope_decay_level{};
    byte envelope_divider{};

    byte sweep_divider_load{};
    bool sweep_enabled{};
    bool sweep_negate{};
    bool sweep_reload{false};
    byte sweep_shift_count{};
    byte sweep_counter{};

    bool use_twos_complement;

    byte volume{};

    void UpdateVolume(const byte volume) {
        volume_reload = volume & 0x0F;
        constant_volume = (volume & 0x10) != 0x00;
        length_counter_halt = (volume & 0x20) != 0x00;
        duty_cycle = DUTY_CYCLES[(volume & 0xC0) >> 6];
    }

    void UpdateSweep(const byte sweep) {
        sweep_enabled = (sweep & 0x80) != 0x00;
        sweep_divider_load = (sweep & 0x70) >> 4;
        sweep_negate = (sweep & 0x08) != 0x00;
        sweep_shift_count = sweep & 0x07;
        sweep_reload = true;
    }

    void UpdateTimerLow(const byte timer_low) {
        timer_reload = (timer_reload & 0xFF00) | timer_low;
        UpdateTargetPeriod();
    }

    void UpdateTimerHigh(const byte timer_high) {
        length_counter_load = timer_high >> 3;
        if (enabled) {
            LoadLengthCounter();
        }
        timer_reload = (timer_reload & ~0x0700) | (static_cast<word>(timer_high & 0x07) << 8);
        UpdateTargetPeriod();
        envelope_start = true;
        duty_counter_bit = 0x00;
    }

    void UpdateTargetPeriod() {
        const word change = timer_reload >> sweep_shift_count;
        if (sweep_negate) {
            target_period = (change > timer) ? 0 : (timer_reload - change);
            if (!use_twos_complement && target_period > 0) {
                target_period--;
            }
        } else {
            target_period = timer_reload + change;
        }
    }
};

class ApuTriangle {
public:
    byte length_counter{};
    bool enabled{false};

    [[nodiscard]] byte GetSample() const {
        if (length_counter == 0x00 || linear_counter == 0x00) { return 0x00; }
        return sequence;
    }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0: UpdateCounter(data); break;
            case 1: break;
            case 2: UpdateTimerLow(data); break;
            case 3: UpdateTimerHigh(data); break;
            default: break;
        }
    }

    void ClockTimer() {
        if (timer == 0x00) {
            timer = timer_reload;
            sequence = (sequence + static_cast<byte>(direction)) & 0xF;
            if (sequence == 15) {
                direction = -1;
            } else if (sequence == 0) {
                direction = +1;
            }
        } else {
            timer--;
        }
    }

    void ClockLengthCounter() {
        if (!length_counter_halt && length_counter != 0x00) {
            length_counter--;
        }
    }

    void ClockLinearCounter() {
        if (linear_counter_reload) {
            linear_counter = linear_counter_load;
        } else if (linear_counter != 0x00) {
            linear_counter--;
        }

        if (!length_counter_halt) {
            linear_counter_reload = false;
        }
    }

    void LoadLengthCounter(const byte value) {
        length_counter = value;
    }

    void LoadLengthCounter() {
        LoadLengthCounter(LENGTH_COUNTER_LOADS[length_counter_load]);
    }

private:
    word timer{}, timer_reload{};
    byte linear_counter_load{}, linear_counter{};
    byte length_counter_load{};

    byte sequence{15};
    int direction{-1};

    bool linear_counter_reload{false}, length_counter_halt{false};

    void UpdateCounter(const byte data) {
        length_counter_halt = (data & 0x80) != 0x00;
        linear_counter_load = data & 0x7F;
    }

    void UpdateTimerLow(const byte data) {
        timer_reload = (timer_reload & 0xFF00) | static_cast<word>(data);
    }

    void UpdateTimerHigh(const byte data) {
        length_counter_load = (data & 0xF8) >> 3;
        if (enabled) {
            LoadLengthCounter();
        }
        timer_reload = (timer_reload & ~0x0700) | (static_cast<word>(data & 0x7) << 8);
        linear_counter_reload = true;
    }
};

enum class ApuChannel: byte {
    Pulse1 = (1 << 0),
    Pulse2 = (1 << 1),
    Triangle = (1 << 2),
    Noise = (1 << 3),
    Dmc = (1 << 4),
};

class Apu {
public:
    explicit Apu(std::shared_ptr<AudioQueue> sink, InterruptRequestFlag irq_requested): sink{std::move(sink)}, irq_requested(std::move(irq_requested)) {}

    void Tick(uint64_t cpu_cycles);

    [[maybe_unused]] static void Reset() { spdlog::error("Reset not implemented for APU"); }

    [[nodiscard]] byte CpuRead(word address);
    void CpuWrite(word address, byte data);

    friend class Debugger;

private:
    std::shared_ptr<AudioQueue> sink{};

    ApuPulse pulse_1{false}, pulse_2{true};
    ApuTriangle triangle{};

    InterruptRequestFlag irq_requested{};

    uint64_t frame_begin_cpu_cycle{0x00};
    FrameCounterStepMode step_mode{FrameCounterStepMode::FourStep};
    bool raise_irq{false};

    byte prev_enabled_channels{0x00};

    void UpdateFrameCounter(byte data);
    static float Mix(byte pulse1_sample, byte pulse2_sample, byte triangle_sample);

    static bool ChannelEnabled(const byte reg, ApuChannel channel) {
        return (reg & static_cast<byte>(channel)) != 0x00;
    }
};
