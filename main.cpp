#include <filesystem>
#include <iostream>

#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>
#include <args.hxx>

#include "sen.hxx"
#include "util.hxx"

RomArgs read_cmd_args(int argc, char** argv);

int main(int argc, char** argv) {
    spdlog::cfg::load_env_levels();

    auto rom_args = read_cmd_args(argc, argv);
}

RomArgs read_cmd_args(int argc, char** argv) {
    args::ArgumentParser parser("sen is a NES emulator", "");
    args::Positional<std::string> rom_path(parser, "rom", "Path to ROM file",
                                           args::Options::Required);

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        std::exit(-1);
    } catch (args::ValidationError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        std::exit(-1);
    }

    std::filesystem::path rom_file_path;
    if (rom_path) {
        rom_file_path = std::filesystem::path(args::get(rom_path));

        if (!std::filesystem::exists(rom_file_path)) {
            spdlog::error("ROM file {} does not exist. Exiting...", rom_file_path.string());
            std::exit(-1);
        }
    }

    auto rom = read_binary_file(rom_file_path);

    return {rom};
}
