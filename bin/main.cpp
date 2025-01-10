#include <../cmake-build-debug/vcpkg_installed/x64-linux/include/nfd.h>
#include <../cmake-build-debug/vcpkg_installed/x64-linux/include/spdlog/cfg/env.h>

#include "ui.hxx"

int main(int, char**) {
    spdlog::cfg::load_env_levels();
    NFD_Init();

    Ui ui{};
    ui.MainLoop();

    NFD_Quit();

    return 0;
}
