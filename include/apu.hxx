//
// Created by rudolph on 30/11/24.
//

#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <spdlog/spdlog.h>

#include "constants.hxx"

constexpr std::array<byte, 4> DUTY_CYCLES = {
    0b10000000,
    0b11000000,
    0b11110000,
    0b00111111,
};

constexpr std::array<byte, 32> LENGTH_COUNTER_LOADS{10, 254, 20,  2,  40, 4,  80, 6,  160, 8,  60,
                                         10, 14,  12,  26, 14, 12, 16, 24, 18,  48, 20,
                                         96, 22,  192, 24, 72, 26, 16, 28, 32,  30};

constexpr std::array<word, 16> NOISE_TIMER_CPU_CYCLES =
    {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

enum class FrameCounterStepMode: uint8_t {
    FourStep,
    FiveStep,
};

class AudioQueue {
  public:
    virtual ~AudioQueue() = default;

    virtual void push(float sample) = 0;
};

struct LengthCounter {
    bool* channel_enabled;
    byte counter{0x00};
    byte counter_load{};

    bool halt{false};

    explicit LengthCounter(bool* channel_enabled_ptr) : channel_enabled{channel_enabled_ptr} {}

    void Load(const byte value) {
        counter = value;
    }

    void Load() {
        Load(LENGTH_COUNTER_LOADS.at(counter_load));
    }

    void Clock() {
        if (!halt && counter != 0x00) {
            counter--;
        }
    }

    void UpdateLengthCounterLoad(const byte value) {
        counter_load = value;
        if (*channel_enabled) {
            Load();
        }
    }
};

struct SweepUnit {
    word& timer_reload;

    word target_period{};

    byte sweep_divider_load{};
    byte sweep_shift_count{};
    byte sweep_counter{};

    bool sweep_enabled{};
    bool sweep_negate{};
    bool sweep_reload{false};
    bool use_twos_complement{};

    explicit SweepUnit(word& timer_reload, const bool use_twos_complement) :
        timer_reload{timer_reload},
        use_twos_complement{use_twos_complement} {}

    void Update(const byte sweep) {
        sweep_enabled = (sweep & 0x80) != 0x00;
        sweep_divider_load = (sweep & 0x70) >> 4;
        sweep_negate = (sweep & 0x08) != 0x00;
        sweep_shift_count = sweep & 0x07;
        sweep_reload = true;
    }

    void Clock(const word timer) {
        UpdateTargetPeriod(timer);

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

    void UpdateTargetPeriod(const word timer) {
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

struct EnvelopeGenerator {
    byte decay_level{};
    byte divider{};
    bool start{false};

    void Clock(const byte volume_reload, const LengthCounter& length_counter) {
        if (!start) {
            if (divider == 0x00) {
                divider = volume_reload;
                if (decay_level == 0x00 && length_counter.halt) {
                    decay_level = 15;
                } else if (decay_level != 0x00) {
                    decay_level--;
                }
            }
        } else {
            start = false;
            decay_level = 15;
            divider = volume_reload;
        }
    }
};

class ApuPulse {
  public:
    SweepUnit sweep_unit;
    LengthCounter length_counter;
    EnvelopeGenerator envelope_generator;

    bool enabled{false};

    explicit ApuPulse(const bool use_twos_complement) :
        sweep_unit(timer_reload, use_twos_complement),
        length_counter(&enabled) {}

    [[nodiscard]] byte GetSample() const {
        if (timer < 8 || length_counter.counter == 0x00 || target_period > 0x7FF) {
            return 0x00;
        }

        const auto duty = (duty_cycle & (1 << duty_counter_bit)) >> duty_counter_bit;
        return duty * (constant_volume ? volume_reload : envelope_generator.decay_level);
    }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0:
                UpdateVolume(data);
                break;
            case 1:
                sweep_unit.Update(data);
                break;
            case 2:
                UpdateTimerLow(data);
                break;
            case 3:
                UpdateTimerHigh(data);
                break;
            default:
                break;
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
        envelope_generator.Clock(volume_reload, length_counter);
    }

    void ClockLengthCounter() {
        length_counter.Clock();
    }

    void ClockSweep() {
        sweep_unit.Clock(timer);
    }

  private:
    word timer{}, timer_reload{}, target_period{};

    byte duty_counter_bit{0x00};

    byte duty_cycle{};
    bool constant_volume{false};
    byte volume_reload{};

    byte volume{};

    void UpdateVolume(const byte volume) {
        volume_reload = volume & 0x0FU;
        constant_volume = (volume & 0x10U) != 0x00;
        length_counter.halt = (volume & 0x20U) != 0x00;
        duty_cycle = DUTY_CYCLES.at((volume & 0xC0U) >> 6U);
    }

    void UpdateTimerLow(const byte timer_low) {
        timer_reload = (timer_reload & 0xFF00) | timer_low;
        sweep_unit.UpdateTargetPeriod(timer);
    }

    void UpdateTimerHigh(const byte timer_high) {
        length_counter.UpdateLengthCounterLoad(timer_high >> 3);
        timer_reload = (timer_reload & ~0x0700) | (static_cast<word>(timer_high & 0x07) << 8);
        sweep_unit.UpdateTargetPeriod(timer);
        envelope_generator.start = true;
        duty_counter_bit = 0x00;
    }
};

class ApuTriangle {
  public:
    LengthCounter length_counter;

    bool enabled{false};

    ApuTriangle() : length_counter{&enabled} {}

    [[nodiscard]] byte GetSample() const {
        if (length_counter.counter == 0x00 || linear_counter == 0x00) {
            return 0x00;
        }
        return sequence;
    }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0:
                UpdateCounter(data);
                break;
            case 1:
                break;
            case 2:
                UpdateTimerLow(data);
                break;
            case 3:
                UpdateTimerHigh(data);
                break;
            default:
                break;
        }
    }

    void ClockTimer() {
        if (timer == 0x00) {
            timer = timer_reload;
            sequence = static_cast<byte>(static_cast<int>(sequence) + direction) & 0xFU;
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
        length_counter.Clock();
    }

    void ClockLinearCounter() {
        if (linear_counter_reload) {
            linear_counter = linear_counter_load;
        } else if (linear_counter != 0x00) {
            linear_counter--;
        }

        if (!length_counter.halt) {
            linear_counter_reload = false;
        }
    }

  private:
    int direction{-1};
    word timer{}, timer_reload{};
    byte linear_counter{}, linear_counter_load{};
    byte sequence{15};
    bool linear_counter_reload{false};

    void UpdateCounter(const byte data) {
        length_counter.halt = (data & 0x80) != 0x00;
        linear_counter_load = data & 0x7F;
    }

    void UpdateTimerLow(const byte data) {
        timer_reload = (timer_reload & 0xFF00) | static_cast<word>(data);
    }

    void UpdateTimerHigh(const byte data) {
        length_counter.UpdateLengthCounterLoad((data & 0xF8) >> 3);
        timer_reload = (timer_reload & ~0x0700) | (static_cast<word>(data & 0x7) << 8);
        linear_counter_reload = true;
    }
};

class ApuNoise {
  public:
    EnvelopeGenerator envelope_generator;
    LengthCounter length_counter;

    word shift_register{1}, timer_reload{0x00}, timer{};

    byte volume_reload{};
    bool enabled{}, constant_volume{false}, mode_1{false};

    ApuNoise() : length_counter{&enabled} {}

    [[nodiscard]] byte GetSample() const {
        if ((shift_register & 0b1) != 0 || length_counter.counter == 0) {
            return 0x00;
        }

        return constant_volume ? volume_reload : envelope_generator.decay_level;
    }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0:
                UpdateCounter(data);
                break;
            case 1:
                break;
            case 2:
                UpdateModeAndPeriod(data);
                break;
            case 3:
                UpdateLengthCounterLoad(data);
                break;
            default:
                break;
        }
    }

    void ClockEnvelope() {
        envelope_generator.Clock(volume_reload, length_counter);
    }

    void ClockLengthCounter() {
        length_counter.Clock();
    }

    void ClockTimer() {
        if (timer == 0x00) {
            timer = timer_reload;

            const word feedback_bit = shift_register ^ (mode_1 ? 1 : 0);
            shift_register >>= 1;
            shift_register |= (feedback_bit << 14);
        } else {
            timer--;
        }
    }

  private:
    void UpdateCounter(const byte value) {
        length_counter.halt = (value & 0x20) != 0x00;
        constant_volume = (value & 0x10) != 0x00;
        volume_reload = value & 0x0F;
    }

    void UpdateModeAndPeriod(const byte value) {
        mode_1 = (value & 0x80) != 0x00;
        timer_reload = NOISE_TIMER_CPU_CYCLES.at(value & 0x0F);
    }

    void UpdateLengthCounterLoad(const byte value) {
        length_counter.UpdateLengthCounterLoad((value & 0xF8) >> 3);
        envelope_generator.start = true;
    }
};

class ApuDmc {
  public:
    bool enabled{false};

    explicit ApuDmc(InterruptRequestFlag irq_flag) :
        irq_flag{std::move(irq_flag)} {}

    byte get_sample() {
        return 0x00;
    }

    void WriteRegister(const byte offset, const byte data) {
        switch (offset) {
            case 0:
                update_irq_loop_freq(data);
                break;
            case 1:
                load_counter(data);
                break;
            case 2:
                load_sample_address(data);
                break;
            case 3:
                load_sample_length(data);
                break;
            default:
                break;
        }
    }

private:
    InterruptRequestFlag irq_flag;

    word sample_start_address{}, sample_length{};

    byte frequency{};
    byte counter{};

    bool irq_enable{}, loop{};

    void update_irq_loop_freq(const byte data) {
        irq_enable = (data & 0x80) != 0x00;
        loop = (data & 0x40) != 0x00;
        frequency = data & 0x0F;
    }

    void load_counter(const byte data) {
        counter = data & 0x7F;
    }

    void load_sample_address(const byte data) {
        sample_start_address = 0xC000U | (static_cast<word>(data) << 6);
    }

    void load_sample_length(const byte data) {
        sample_length = (static_cast<word>(data) << 4) | 0x0001;
    }
};

enum class ApuChannel : byte {
    Pulse1 = (1U << 0U),
    Pulse2 = (1U << 1U),
    Triangle = (1U << 2U),
    Noise = (1U << 3U),
    Dmc = (1U << 4U),
};

class Apu {
  public:
    explicit Apu(std::shared_ptr<AudioQueue> sink, InterruptRequestFlag irq_requested) :
        audio_queue{std::move(sink)},
        dmc {irq_requested},
        irq_requested(std::move(irq_requested)) {}

    void Tick(uint64_t cpu_cycles);

    [[maybe_unused]] static void Reset() {
        spdlog::error("Reset not implemented for APU");
    }

    [[nodiscard]] byte CpuRead(word address);
    void CpuWrite(word address, byte data);

    friend class Debugger;

  private:
    std::shared_ptr<AudioQueue> audio_queue{};

    ApuPulse pulse_1{false}, pulse_2{true};
    ApuTriangle triangle;
    ApuNoise noise;
    ApuDmc dmc;

    InterruptRequestFlag irq_requested{};

    uint64_t frame_begin_cpu_cycle{0x00};
    FrameCounterStepMode step_mode{FrameCounterStepMode::FourStep};
    bool raise_irq{false};

    byte prev_enabled_channels{0x00};

    void UpdateFrameCounter(byte data);
    static float
    Mix(byte pulse1_sample, byte pulse2_sample, byte triangle_sample, byte noise_sample, byte dmc_sample);

    static bool ChannelEnabled(const byte reg, ApuChannel channel) {
        return (reg & static_cast<byte>(channel)) != 0x00;
    }
};
