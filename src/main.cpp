#include <filesystem>
#include <iostream>

#include <args.hxx>

#include "cartridge.h"
#include "mmu.h"
#include "rom.h"

/*
 * Command-line and Program Options
 * */
struct Options {
    std::filesystem::path romFilePath;
};

Options parseArgs(int argc, char** argv) {
    args::ArgumentParser parser("Sen - NES Emulator",
                                "Sen is a NES Emulator for fun and learning");
    args::HelpFlag help(parser, "help", "Display help menu", {'h', "help"});

    args::Positional<std::string> rom(parser, "rom", "Program ROM",
                                      args::Options::Required);

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::cout << parser;
        std::exit(0);
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        std::exit(-1);
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        std::exit(-1);
    }

    std::string romFilePath;
    if (rom) {
        romFilePath = std::filesystem::path(args::get(rom));
    } else {
        // Should not get to this point
        std::cerr << "Argument rom is required\n";
        std::cerr << parser;
        std::exit(-1);
    }

    return {romFilePath};
}

int main(int argc, char** argv) {
    auto options = parseArgs(argc, argv);

    auto rom = Rom::ParseRomFile(options.romFilePath);
    auto cart = std::make_shared<Cartridge>(std::move(rom));
    auto mmu = std::make_shared<Mmu>(cart);

    return 0;
}
