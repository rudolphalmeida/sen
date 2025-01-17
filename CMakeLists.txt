cmake_minimum_required(VERSION 3.28.0)
project(sen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(libconfig CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_library(sen
    include/constants.hxx
    include/sen.hxx src/sen.cpp
    include/util.hxx src/util.cpp
    include/cartridge.hxx src/cartridge.cpp
    include/mapper.hxx src/mapper.cpp
    include/cpu.hxx include/debugger.hxx
    include/bus.hxx src/bus.cpp
    include/ppu.hxx src/ppu.cpp
    include/controller.hxx
    src/apu.cpp include/apu.hxx
)
target_link_libraries(sen PRIVATE
    spdlog::spdlog
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    fmt::fmt
)
target_include_directories(sen PUBLIC include lib)

add_executable(sen_sdl2
        bin/main.cpp
        bin/ui.hxx
        bin/ui.cpp
        lib/imgui_memory_editor.h
        lib/stb_image_write.h
        lib/stb_image_write.cpp
        lib/crt_core.h
        lib/crt_core.c
        lib/crt_nes.h
        lib/crt_nes.c
        bin/filters.cpp
        lib/IconsFontAwesome6.h
        lib/ImGuiNotify.hpp
        bin/settings.hxx
        bin/filters.hxx
)
target_include_directories(sen_sdl2 PRIVATE include lib bin)
target_link_libraries(sen_sdl2 PRIVATE
    sen
    spdlog::spdlog
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    OpenGL::GL
    nfd::nfd
    imgui::imgui
    libconfig::config libconfig::config++
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

add_executable(cpu_tests tests/flatbus.hxx tests/cpu_tests.cpp)
target_link_libraries(cpu_tests PRIVATE sen Catch2::Catch2WithMain nlohmann_json::nlohmann_json)

include(CTest)
include(Catch)
catch_discover_tests(cpu_tests)