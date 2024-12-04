cmake_minimum_required(VERSION 3.28.0)
project(sen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(libconfig CONFIG REQUIRED)

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

add_library(libsen
    constants.hxx
    sen.hxx sen.cpp
    util.hxx util.cpp
    cartridge.hxx cartridge.cpp
    mapper.hxx mapper.cpp
    cpu.hxx debugger.hxx
    bus.hxx bus.cpp
    ppu.hxx ppu.cpp
    controller.hxx
    apu.cpp apu.hxx)
target_link_libraries(libsen PRIVATE
    spdlog::spdlog
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    fmt::fmt
)
target_include_directories(libsen PUBLIC .)
target_compile_definitions(libsen PUBLIC $<$<CONFIG:RelWithDebInfo>:LOG_DEBUG>)

add_executable(sen main.cpp ui.hxx ui.cpp imgui_memory_editor.h stb_image_write.h stb_image_write.cpp IconsFontAwesome5.h miniaudio.cpp)
target_include_directories(sen PRIVATE ${MINIAUDIO_INCLUDE_DIRS})
target_link_libraries(sen PRIVATE
    libsen
    spdlog::spdlog
    spdlog::spdlog_header_only
    nlohmann_json::nlohmann_json
    glfw
    OpenGL::GL
    nfd::nfd
    imgui::imgui
    libconfig::config libconfig::config++
    dl pthread m
)

add_executable(cpu_tests tests/flatbus.hxx tests/cpu_tests.cpp)
target_link_libraries(cpu_tests PRIVATE libsen Catch2::Catch2WithMain nlohmann_json::nlohmann_json)

include(CTest)
include(Catch)
catch_discover_tests(cpu_tests)