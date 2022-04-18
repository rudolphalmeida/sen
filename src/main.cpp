#include <filesystem>
#include <iostream>

#include <spdlog/cfg/env.h>

#include "cartridge.h"
#include "cpu.h"
#include "mmu.h"
#include "options.hxx"
#include "rom.h"
#include "ui.hxx"

int main(int argc, char** argv) {
    spdlog::cfg::load_env_levels();
    auto options = parseArgs(argc, argv);

    auto rom = Rom::ParseRomFile(options.romFilePath);
    auto cart = std::make_shared<Cartridge>(std::move(rom));
    auto mmu = std::make_shared<Mmu>(cart);

    Cpu cpu(mmu);

    Ui ui{std::move(options), std::move(cpu), mmu};

    return ui.Run();
}
