#include <iostream>

#include <args.hxx>

int main(int argc, char** argv) {
    args::ArgumentParser parser("Sen - NES Emulator",
                                "Sen is a NES Emulator for fun and learning");
    args::HelpFlag help(parser, "help", "Display help menu", {'h', "help"});

    args::Positional<std::string> rom(parser, "rom", "Program ROM");

    args::CompletionFlag completion(parser, {"complete"});

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (rom) {
        std::cout << "Got path: " << args::get(rom) << "\n";
    }

    return 0;
}
