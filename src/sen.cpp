#include "sen.hxx"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "apu.hxx"
#include "bus.hxx"
#include "cartridge.hxx"
#include "constants.hxx"
#include "controller.hxx"
#include "cpu.hxx"
#include "mapper.hxx"
#include "ppu.hxx"

Sen::Sen(const RomArgs& rom_args, const std::shared_ptr<AudioQueue>& sink) {
    nmi_requested = std::make_shared<bool>(false);
    irq_requested = std::make_shared<bool>(false);

    auto cartridge = ParseRomFile(rom_args);
    ppu = std::make_shared<Ppu>(cartridge, nmi_requested);
    apu = std::make_shared<Apu>(sink, irq_requested);
    controller = std::make_shared<Controller>();

    bus = std::make_shared<Bus>(std::move(cartridge), ppu, apu, controller);
    cpu = Cpu<Bus>(bus, nmi_requested, irq_requested);
}

void Sen::RunForCycles(const uint64_t cycles) {
    if (!running) {
        running = true;
        cpu.start();
    }

    const auto cpu_cycles = bus->cycles;
    const auto target_cycles = cpu_cycles + cycles - carry_over_cycles;

    while (bus->cycles < target_cycles) {
        cpu.step();
    }

    carry_over_cycles = bus->cycles - target_cycles;
}

void Sen::StepOpcode() {
    if (!running) {
        running = true;
        cpu.start();
    }

    cpu.step();
}

void Sen::RunForOneScanline() {
    if (!running) {
        running = true;
        cpu.start();
    }

    const auto ppu_start_scanline = ppu->Scanline();

    cpu.step();
    while (ppu->Scanline() == ppu_start_scanline) {
        cpu.step();
    }
}

void Sen::RunForOneFrame() {
    if (!running) {
        running = true;
        cpu.start();
    }

    const auto cpu_cycles = bus->cycles;
    const auto target_cycles = cpu_cycles + CYCLES_PER_FRAME - carry_over_cycles;

    while (bus->cycles < target_cycles) {
        cpu.step();
    }

    carry_over_cycles = bus->cycles - target_cycles;
}

void Sen::set_pressed_keys(const ControllerPort port, const byte key) const {
    controller->set_pressed_keys(port, key);
}

std::shared_ptr<Cartridge> ParseRomFile(const RomArgs& rom_args) {
    auto rom_iter = rom_args.rom.cbegin();

    if (*(rom_iter + 0) != '\x4E' || *(rom_iter + 1) != '\x45' || *(rom_iter + 2) != '\x53'
        || *(rom_iter + 3) != '\x1A') {
        // ROM should begin with NES\x1A
        spdlog::error("Provided file is not a valid NES ROM");
        std::exit(-1);
    }
    std::advance(rom_iter, 4);

    size_t prg_rom_banks = *rom_iter++;
    size_t chr_rom_banks = *rom_iter++;

    if (chr_rom_banks == 0x00) {
        spdlog::info("Cartridge uses CHR-RAM");
    }

    const auto flag_6 = *rom_iter++;
    Mirroring mirroring{};
    if ((flag_6 & 0b1000U) != 0U) {
        spdlog::debug("TODO: Cartridge uses alternative nametable layout");
        mirroring = Mirroring::FourScreenVram;
    } else if ((flag_6 & 0b1U) != 0U) {
        mirroring = Mirroring::Vertical;
    } else {
        mirroring = Mirroring::Horizontal;
    }

    const bool battery_backed_ram = (flag_6 & 0b10) == 0b10;
    if (battery_backed_ram) {
        // TODO: If battery_backed_ram is True, check for RAM provided in rom_args
        spdlog::info("Cartridge uses battery backed RAM");
    }

    word mapper_number;

    const auto flag_7 = *rom_iter++;
    const bool nes2_0_format = (flag_7 & 0x0C) == 0x08;

    size_t prg_ram_size = 0U;

    if (nes2_0_format) {
        spdlog::info("ROM is in NES2.0 format");

        const auto flag_8 = *rom_iter++;
        mapper_number = ((flag_8 & 0x0F) << 8) | (flag_7 & 0xF0) | ((flag_6 & 0xF0) >> 4);

        const auto submapper = (flag_8 & 0xF0) >> 4;
        spdlog::info("ROM has sub mapper: {}", submapper);

        const auto flag_9 = *rom_iter++;
        prg_rom_banks |= ((flag_9 & 0x0F) << 8);
        chr_rom_banks |= ((flag_9 & 0xF0) << 8);

        // TODO: Ignore the rest of the NES2.0 stuff for now
        std::advance(rom_iter, 16 - 10);
    } else {
        spdlog::info("ROM is in iNES format");

        mapper_number = (flag_7 & 0xF0) | ((flag_6 & 0xF0) >> 4);

        const auto flag_8 = *rom_iter++;
        prg_ram_size = flag_8 ? flag_8 * 8192 : 8192;
        spdlog::info("PRG RAM size (Bytes): {}", prg_ram_size);

        // Ignore bytes 9-15 for iNES
        std::advance(rom_iter, 16 - 9);
    }

    size_t prg_rom_size = 16384 * prg_rom_banks;
    size_t chr_rom_size = 8192 * chr_rom_banks;
    spdlog::info("PRG ROM size (Bytes): {}", prg_rom_size);
    spdlog::info("CHR ROM size (Bytes): {}", chr_rom_size);

    if (chr_rom_size == 0x00 && !nes2_0_format) {
        spdlog::info("Cartridge uses CHR-RAM");
    }

    const RomHeader header{
        .prg_rom_size = prg_rom_size,
        .prg_rom_banks = prg_rom_banks,
        .chr_rom_size = chr_rom_size,
        .chr_rom_banks = chr_rom_banks,
        .prg_ram_size = prg_ram_size,
        .hardware_mirroring = mirroring,
        .mapper_number = mapper_number,
        .battery_backed_ram = battery_backed_ram
    };

    // Check if a 512-byte trainer is present and is so, ignore it
    if ((flag_6 & 0b100U) != 0) {
        std::advance(rom_iter, 512);
    }

    // Read PRG-ROM
    std::vector<byte> prg_rom;
    prg_rom.reserve(prg_rom_size);
    std::copy_n(rom_iter, prg_rom_size, std::back_inserter(prg_rom));
    std::advance(rom_iter, prg_rom_size);

    // Read CHR-ROM
    std::vector<byte> chr_rom;
    chr_rom.reserve(chr_rom_size);
    std::copy_n(rom_iter, chr_rom_size, std::back_inserter(chr_rom));
    std::advance(rom_iter, chr_rom_size);

    return init_mapper(header, std::move(prg_rom), std::move(chr_rom));
}
