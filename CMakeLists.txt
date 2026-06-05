cmake_minimum_required(VERSION 3.10)

project(rocket)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_executable(debug main.cpp rocket.cpp LoadKspParts.cpp parts.cpp)
target_include_directories(debug PRIVATE ext)
target_compile_options(debug PRIVATE -Wall -Wunused-value -Wshadow -pedantic)
