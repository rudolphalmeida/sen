cmake_minimum_required(VERSION 3.21)
project(sen)

set(CMAKE_CXX_STANDARD 20)

set(SRC_FILES src/utils.h src/rom.cpp src/rom.h src/utils.cpp src/cartridge.cpp src/cartridge.h src/memory.h src/mmu.cpp src/mmu.h)

find_package(args CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(sen src/main.cpp ${SRC_FILES})
target_include_directories(sen PRIVATE src)
target_link_libraries(sen PRIVATE taywee::args spdlog::spdlog spdlog::spdlog_header_only)
