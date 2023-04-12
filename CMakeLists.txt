cmake_minimum_required(VERSION 3.0.0)
project(sen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

find_package(args CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_library(libsen
    constants.hxx
    tests/flatbus.hxx
    sen.hxx sen.cpp
    util.hxx util.cpp
    cartridge.hxx cartridge.cpp
    mapper.hxx mapper.cpp
    cpu.hxx cpu.cpp
    bus.hxx bus.cpp
    ppu.hxx ppu.cpp)
target_link_libraries(libsen PRIVATE
    spdlog::spdlog
    spdlog::spdlog_header_only
    taywee::args
    nlohmann_json::nlohmann_json)
target_include_directories(libsen PUBLIC .)

add_executable(sen main.cpp)
target_link_libraries(sen PRIVATE
    libsen
    spdlog::spdlog
    spdlog::spdlog_header_only
    taywee::args
    nlohmann_json::nlohmann_json
)

add_executable(cpu_tests tests/cpu_tests.cpp)
target_link_libraries(cpu_tests PRIVATE libsen Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
