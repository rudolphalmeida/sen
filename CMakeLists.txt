cmake_minimum_required(VERSION 3.0.0)
project(sen VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

find_package(args CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(sen
    main.cpp constants.hxx
    sen.hxx sen.cpp
    util.hxx util.cpp
    cartridge.hxx cartridge.cpp
    mapper.hxx mapper.cpp
)
target_link_libraries(sen PRIVATE spdlog::spdlog spdlog::spdlog_header_only taywee::args)
