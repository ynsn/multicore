cmake_minimum_required(VERSION 3.24)
include(CheckCCompilerFlag)
include(FetchContent)

project(multicore
        LANGUAGES CXX
        VERSION 0.0.1
        DESCRIPTION "Multicore Library")

option(MTC_ENABLE_TESTS "Builds the tests" ON)
option(MTC_ENABLE_DOCS "Generates the Doxygen documentation" ON)

message(STATUS "Enabled tests: ${MTC_ENABLE_TESTS}")
message(STATUS "Enabled docs: ${MTC_ENABLE_DOCS}")

set(CMAKE_CXX_STANDARD 20)

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

if (MTC_ENABLE_DOCS)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        message(STATUS "Doxygen found, building docs is supported")
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_SOURCE_DIR}/docs/out/Doxyfile)
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(multicore_docs ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/out/docs
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM)
    else ()
        message(FATAL_ERROR "Doxygen not found, building docs is not supported")
    endif ()
endif ()


