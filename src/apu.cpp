//
// Created by rudolph on 30/11/24.
//

#include <cstdint>

#include "apu.hxx"
#include "util.hxx"

void Apu::Tick(const uint64_t cpu_cycles) {
    const uint64_t cpu_cycles_into_frame = cpu_cycles - frame_begin_cpu_cycle;

    if (cpu_cycles_into_frame == 7457) {
        // TODO: Step 1
        //       Clock envelopes and triangle linear counter
        pulse_1.ClockEnvelope();
        pulse_2.ClockEnvelope();
        triangle.ClockLinearCounter();
        noise.ClockEnvelope();
    }

    if (cpu_cycles_into_frame == 14913) {
        // TODO: Step 2
        //       Clock envelopes and triangle linear counter
        //       Clock length counters and sweep units
        pulse_1.ClockEnvelope();
        pulse_1.ClockLengthCounter();
        pulse_1.ClockSweep();
        pulse_2.ClockEnvelope();
        pulse_2.ClockLengthCounter();
        pulse_2.ClockSweep();
        triangle.ClockLinearCounter();
        triangle.ClockLengthCounter();
        noise.ClockEnvelope();
        noise.ClockLengthCounter();
    }

    if (cpu_cycles_into_frame == 22371) {
        // TODO: Step 3
        //       Clock envelopes and triangle linear counter
        pulse_1.ClockEnvelope();
        pulse_2.ClockEnvelope();
        triangle.ClockLinearCounter();
        noise.ClockEnvelope();
    }

    if (step_mode == FrameCounterStepMode::FourStep && cpu_cycles_into_frame == 29828
        && raise_irq) {
        *irq_requested = true;
    }

    if (cpu_cycles_into_frame == 29829) {
        // TODO: Step 4
        //       Clock envelopes and triangle linear counter if 4 step
        //       Clock length counters and sweep units if 4 step
        pulse_1.ClockEnvelope();
        pulse_1.ClockLengthCounter();
        pulse_1.ClockSweep();
        pulse_2.ClockEnvelope();
        pulse_2.ClockLengthCounter();
        pulse_2.ClockSweep();
        triangle.ClockLinearCounter();
        triangle.ClockLengthCounter();
        noise.ClockEnvelope();
        noise.ClockLengthCounter();

        *irq_requested = raise_irq;
    }

    if (step_mode == FrameCounterStepMode::FourStep && cpu_cycles_into_frame == 29830) {
        // TODO: 4 step frame end
        frame_begin_cpu_cycle = cpu_cycles;
        *irq_requested = raise_irq;
    }

    if (step_mode == FrameCounterStepMode::FiveStep && cpu_cycles_into_frame == 37281) {
        // TODO: Step 5
        //       Clock envelopes and triangle linear counter
        //       Clock length counters and sweep units
        pulse_1.ClockEnvelope();
        pulse_1.ClockLengthCounter();
        pulse_1.ClockSweep();
        pulse_2.ClockEnvelope();
        pulse_2.ClockLengthCounter();
        pulse_2.ClockSweep();
        triangle.ClockLinearCounter();
        triangle.ClockLengthCounter();
        noise.ClockEnvelope();
        noise.ClockLengthCounter();
    }

    if (step_mode == FrameCounterStepMode::FiveStep && cpu_cycles_into_frame == 37282) {
        frame_begin_cpu_cycle = cpu_cycles;
    }

    if ((cpu_cycles & 0b1) == 0x00) {
        // Pulse timers are updated every APU cycle
        pulse_1.ClockTimer();
        pulse_2.ClockTimer();
    };

    // Triangle timers are updated every CPU cycle
    triangle.ClockTimer();
    noise.ClockTimer();

    const auto pulse1_sample = pulse_1.GetSample();
    const auto pulse2_sample = pulse_2.GetSample();
    const auto triangle_sample = triangle.GetSample();
    const auto noise_sample = noise.GetSample();

    sink->PushSample(Mix(pulse1_sample, pulse2_sample, triangle_sample, noise_sample));
}

byte Apu::CpuRead(const word address) {
    if (address == 0x4015) {
        byte res = 0x00;
        if (pulse_1.length_counter.counter > 0x00) {
            res |= 0x01;
        }
        if (pulse_2.length_counter.counter > 0x00) {
            res |= 0x02;
        }
        if (triangle.length_counter.counter > 0x00) {
            res |= 0x04;
        }
        if (noise.length_counter.counter > 0x00) {
            res |= 0x08;
        }

        if (raise_irq) {
            res |= 0x40;
        }

        raise_irq = false;

        return res;
    }
    return 0xFF;
}

void Apu::CpuWrite(const word address, const byte data) {
    if (InRange<word>(0x4000, address, 0x4003)) {
        pulse_1.WriteRegister(address - 0x4000, data);
    } else if (InRange<word>(0x4004, address, 0x4007)) {
        pulse_2.WriteRegister(address - 0x4004, data);
    } else if (InRange<word>(0x4008, address, 0x400B)) {
        triangle.WriteRegister(address - 0x4008, data);
    } else if (InRange<word>(0x400C, address, 0x400F)) {
        noise.WriteRegister(address - 0x400C, data);
    } else if (address == 0x4015) {
        if (ChannelEnabled(prev_enabled_channels, ApuChannel::Pulse1)
            && !ChannelEnabled(data, ApuChannel::Pulse1)) {
            pulse_1.length_counter.Load(0);
            pulse_1.enabled = false;
        } else if (!ChannelEnabled(prev_enabled_channels, ApuChannel::Pulse1)
                   && ChannelEnabled(data, ApuChannel::Pulse1)) {
            pulse_1.enabled = true;
        }

        if (ChannelEnabled(prev_enabled_channels, ApuChannel::Pulse2)
            && !ChannelEnabled(data, ApuChannel::Pulse2)) {
            pulse_2.length_counter.Load(0);
            pulse_2.enabled = false;
        } else if (!ChannelEnabled(prev_enabled_channels, ApuChannel::Pulse2)
                   && ChannelEnabled(data, ApuChannel::Pulse2)) {
            pulse_2.enabled = true;
        }

        if (ChannelEnabled(prev_enabled_channels, ApuChannel::Triangle)
            && !ChannelEnabled(data, ApuChannel::Triangle)) {
            triangle.length_counter.Load(0);
            triangle.enabled = false;
        } else if (!ChannelEnabled(prev_enabled_channels, ApuChannel::Triangle)
                   && ChannelEnabled(data, ApuChannel::Triangle)) {
            triangle.enabled = true;
        }

        if (ChannelEnabled(prev_enabled_channels, ApuChannel::Noise)
            && !ChannelEnabled(data, ApuChannel::Noise)) {
            noise.length_counter.Load(0);
            noise.enabled = false;
        } else if (!ChannelEnabled(prev_enabled_channels, ApuChannel::Noise)
                   && ChannelEnabled(data, ApuChannel::Noise)) {
            noise.enabled = true;
        }

        prev_enabled_channels = data;
    } else if (address == 0x4017) {
        // TODO: If the write occurs during an APU cycle, the effects occur 3 CPU cycles after
        //       the $4017 write cycle, and if the write occurs between APU cycles,
        //       the effects occurs 4 CPU cycles after the write cycle.
        UpdateFrameCounter(data);
        if (step_mode == FrameCounterStepMode::FiveStep) {
            pulse_1.ClockEnvelope();
            pulse_1.ClockSweep();
            pulse_1.ClockLengthCounter();
            pulse_2.ClockEnvelope();
            pulse_2.ClockSweep();
            pulse_2.ClockLengthCounter();
        }
    }
}

void Apu::UpdateFrameCounter(const byte data) {
    step_mode =
        (data & 0x80) != 0x00 ? FrameCounterStepMode::FiveStep : FrameCounterStepMode::FourStep;
    raise_irq = (data & 0x40) == 0x00;
}

float Apu::Mix(
    const byte pulse1_sample,
    const byte pulse2_sample,
    const byte triangle_sample,
    const byte noise_sample
) {
    float pulse_out = 0.0f;
    if (pulse1_sample != 0x00 || pulse2_sample != 0x00) {
        pulse_out =
            95.88f / ((8128.0f / static_cast<float>(pulse1_sample + pulse2_sample)) + 100.0f);
    }

    const float tnd_out = 0.00851f * static_cast<float>(triangle_sample) + 0.00494 * noise_sample;

    return pulse_out + tnd_out;
}