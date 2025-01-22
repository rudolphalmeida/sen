#include <nfd.h>
#include <spdlog/cfg/env.h>

#include "ui.hxx"

int main(int, char**) {
    spdlog::cfg::load_env_levels();
    NFD_Init();

    Ui ui{};
    ui.MainLoop();

    NFD_Quit();

    return 0;
}
