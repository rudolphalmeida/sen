cmake_minimum_required(VERSION 3.21)
project(sen)

set(CMAKE_CXX_STANDARD 20)

set(SRC_FILES src/utils.h)

find_package(args CONFIG REQUIRED)

add_executable(sen src/main.cpp ${SRC_FILES})
target_include_directories(sen PRIVATE src)
target_link_libraries(sen PRIVATE taywee::args)
