cmake_minimum_required(VERSION 4.0)
project(utf8_ansi_cpp)

set(CMAKE_CXX_STANDARD 20)

find_package(ICU REQUIRED COMPONENTS uc)

add_library(utf8_ansi_cpp SHARED utf8ansi.cpp)

# Proper include dirs for build and install
include(GNUInstallDirs)

target_include_directories(utf8_ansi_cpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link against ICU libraries
target_link_libraries(utf8_ansi_cpp PUBLIC ICU::uc)

# Install rules
install(TARGETS utf8_ansi_cpp
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/utf8ansi.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# -----------------
# Tests
# -----------------
include(CTest)

# Use locally installed packages via CMake config
find_package(GTest CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

enable_testing()

add_executable(utf8_ansi_cpp_tests
    tests/test_utf8_ansi.cpp
)

target_link_libraries(utf8_ansi_cpp_tests PRIVATE utf8_ansi_cpp GTest::gtest_main spdlog::spdlog_header_only)

add_test(NAME utf8_ansi_cpp_tests COMMAND utf8_ansi_cpp_tests)
