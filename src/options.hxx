//
// Created by rudolph on 18/4/22.
//

#ifndef SEN_SRC_OPTIONS_HXX_
#define SEN_SRC_OPTIONS_HXX_

#include <filesystem>

#include <args.hxx>

/*
 * Command-line and Program Options
 * */
struct Options {
    std::filesystem::path romFilePath;
};

Options parseArgs(int argc, char** argv);

#endif  // SEN_SRC_OPTIONS_HXX_
