cmake_minimum_required(VERSION 3.24)
include(CheckCCompilerFlag)
include(FetchContent)

project(multicore
        LANGUAGES CXX
        VERSION 0.0.1
        DESCRIPTION "Multicore Library")

set(CMAKE_CXX_STANDARD 20)

option(MTC_ENABLE_TESTS "Builds the tests" ON)

message(STATUS "Enabled tests: ${MTC_ENABLE_TESTS}")

if (MTC_ENABLE_TESTS)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v3.0.1 # or a later release
    )

    FetchContent_MakeAvailable(Catch2)

    include(CTest)
    include(Catch)

    add_executable(tests tests/main.cpp)
    target_include_directories(tests PUBLIC include)
    catch_discover_tests(tests)
endif ()




