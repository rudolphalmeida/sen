#include <spdlog/cfg/env.h>

#include "ui.hxx"

int main(int, char**) {
    spdlog::cfg::load_env_levels();

    Ui ui{};
    ui.run();


    return 0;
}
