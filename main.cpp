#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include "ui.hxx"

int main(int, char**) {
    spdlog::cfg::load_env_levels();

    Ui ui{};

    ui.Run();

    return 0;
}
