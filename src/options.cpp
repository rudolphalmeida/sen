//
// Created by rudolph on 18/4/22.
//

#include "options.hxx"

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
        exit(0);
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        exit(-1);
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        exit(-1);
    }

    std::filesystem::path romFilePath;
    if (rom) {
        romFilePath = std::filesystem::path(args::get(rom));
    } else {
        // Should not get to this point
        std::cerr << "Argument rom is required\n";
        std::cerr << parser;
        exit(-1);
    }

    return {romFilePath};
}
