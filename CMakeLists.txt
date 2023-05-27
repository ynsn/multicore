cmake_minimum_required(VERSION 3.24)
project(multicore)

set(CMAKE_CXX_STANDARD 20)

add_executable(multicore main.cpp)
target_include_directories(multicore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
