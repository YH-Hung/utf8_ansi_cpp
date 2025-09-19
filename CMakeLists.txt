cmake_minimum_required(VERSION 4.0)
project(utf8_ansi_cpp)

set(CMAKE_CXX_STANDARD 20)

find_package(ICU REQUIRED COMPONENTS uc)

add_library(utf8_ansi_cpp SHARED library.cpp)

# Link against ICU libraries
target_link_libraries(utf8_ansi_cpp PUBLIC ICU::uc)
