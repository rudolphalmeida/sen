cmake_minimum_required(VERSION 3.21)
project(sen)

set(CMAKE_CXX_STANDARD 20)

set(SRC_FILES
        src/utils.h
        src/rom.cpp
        src/rom.h
        src/utils.cpp
        src/cartridge.cpp
        src/cartridge.h
        src/memory.h
        src/mmu.cpp
        src/mmu.h
        src/cpu.cpp
        src/cpu.h
        src/opcode.cpp
        src/opcode.h)

find_package(args CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_executable(sen src/main.cpp ${SRC_FILES})
target_include_directories(sen PRIVATE src)
target_link_libraries(sen PRIVATE
        taywee::args
        spdlog::spdlog
        spdlog::spdlog_header_only
        fmt::fmt
        SDL2::SDL2
        SDL2::SDL2main)
target_compile_options(sen PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Werror -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX
        >)