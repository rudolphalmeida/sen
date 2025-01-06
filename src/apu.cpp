//
// Created by rudolph on 30/11/24.
//

#include "apu.hxx"

#include "util.hxx"

void Apu::Tick(const uint64_t cpu_cycles) {
    const uint64_t cpu_cycles_into_frame = cpu_cycles - frame_begin_cpu_cycle;

    if (cpu_cycles_into_frame == 7457) {
        // TODO: Step 1
        //       Clock envelopes and triangle linear counter
    }

    if (cpu_cycles_into_frame == 14913) {
        // TODO: Step 2
        //       Clock envelopes and triangle linear counter
        //       Clock length counters and sweep units
    }

    if (cpu_cycles_into_frame == 22371) {
        // TODO: Step 3
        //       Clock envelopes and triangle linear counter
    }

    if (step_mode == FrameCounterStepMode::FourStep && cpu_cycles_into_frame == 29828 && raise_irq) {
        // TODO: Set IRQ interrupt
        *irq_requested = true;
    }

    if (cpu_cycles_into_frame == 29829) {
        // TODO: Step 4
        //       Clock envelopes and triangle linear counter if 4 step
        //       Clock length counters and sweep units if 4 step

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
    }

    if (step_mode == FrameCounterStepMode::FiveStep && cpu_cycles_into_frame == 37282) {
        frame_begin_cpu_cycle = cpu_cycles;
    }

    if ((cpu_cycles & 0b1) == 0x00) return;

    const auto pulse1_sample = pulse_1.GetSample();
    const auto pulse2_sample = pulse_2.GetSample();
    const auto _sample = mixer.Mix(pulse1_sample, pulse2_sample);
}

byte Apu::CpuRead(const word address) {
    return 0xFF;
}

void Apu::CpuWrite(const word address, const byte data) {
    if (InRange<word>(0x4000, address, 0x4003)) {
        pulse_1.WriteRegister(address - 0x4000, data);
    } else if (InRange<word>(0x4004, address, 0x4007)) {
        pulse_2.WriteRegister(address - 0x4004, data);
    } else if (address == 0x4017) {
        // TODO: If the write occurs during an APU cycle, the effects occur 3 CPU cycles after
        //       the $4017 write cycle, and if the write occurs between APU cycles,
        //       the effects occurs 4 CPU cycles after the write cycle.
        UpdateFrameCounter(data);
    }
}

void Apu::UpdateFrameCounter(const byte data) {
    step_mode = (data & 0x80) != 0x00 ? FrameCounterStepMode::FiveStep : FrameCounterStepMode::FourStep;
    raise_irq = (data & 0x40) == 0x00;
}