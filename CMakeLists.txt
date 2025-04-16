cmake_minimum_required(VERSION 3.28.0)
project(sen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(spdlog CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(libconfig CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(boost_circular_buffer REQUIRED CONFIG)

add_library(sen
        include/constants.hxx
        include/sen.hxx src/sen.cpp
        include/util.hxx src/util.cpp
        include/cartridge.hxx
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
        Boost::circular_buffer
)
target_include_directories(sen PUBLIC include lib)
if (UNIX)
    target_compile_options(sen
            PUBLIC
            -Wall
            -Wpedantic
            -Wextra
            -Werror
    )
endif ()

add_executable(sen_sdl
        bin/main.cpp
        bin/ui.hxx
        bin/ui.cpp
        lib/imgui_memory_editor.h
        lib/crt_core.h
        lib/crt_core.c
        lib/crt_nes.h
        lib/crt_nes.c
        bin/filters.cpp
        lib/IconsFontAwesome6.h
        lib/ImGuiNotify.hpp
        bin/settings.hxx
        bin/filters.hxx
        bin/spdlog_imgui_sink.h
)
target_include_directories(sen_sdl PRIVATE include lib bin)
target_link_libraries(sen_sdl PRIVATE
        sen
        spdlog::spdlog
        spdlog::spdlog_header_only
        nlohmann_json::nlohmann_json
        OpenGL::GL
        imgui::imgui
        SDL3::SDL3
        Boost::circular_buffer

        $<$<PLATFORM_ID:Linux>:libconfig::config libconfig::config++>
        $<$<PLATFORM_ID:Windows>:libconfig::libconfig libconfig::libconfig++>
)

add_custom_command(TARGET sen_sdl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/resources/fa-solid-900.ttf"
        $<TARGET_FILE_DIR:sen_sdl>/fa-solid-900.ttf
)


add_executable(cpu_tests tests/flatbus.hxx tests/cpu_tests.cpp)
target_link_libraries(cpu_tests PRIVATE sen Catch2::Catch2WithMain nlohmann_json::nlohmann_json)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(cpu_tests PRIVATE "/utf-8")
endif ()

include(CTest)
include(Catch)
catch_discover_tests(cpu_tests)
