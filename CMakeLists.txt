cmake_minimum_required(VERSION 4.0)
project(utf8_ansi_cpp)

set(CMAKE_CXX_STANDARD 20)

find_package(ICU REQUIRED COMPONENTS uc)

add_library(utf8_ansi_cpp SHARED library.cpp)

target_include_directories(utf8_ansi_cpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link against ICU libraries
target_link_libraries(utf8_ansi_cpp PUBLIC ICU::uc)

# -----------------
# Tests
# -----------------
include(FetchContent)
include(CTest)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Bring in spdlog (header-only)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

enable_testing()

add_executable(utf8_ansi_cpp_tests
    tests/test_utf8_ansi.cpp
)

target_link_libraries(utf8_ansi_cpp_tests PRIVATE utf8_ansi_cpp GTest::gtest_main spdlog::spdlog_header_only)

add_test(NAME utf8_ansi_cpp_tests COMMAND utf8_ansi_cpp_tests)
