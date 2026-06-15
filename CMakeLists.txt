cmake_minimum_required(VERSION 3.10)

project(rocket)

set(CMAKE_CXX_STANDARD 20)

add_executable(debug main.cpp rocket.cpp LoadKspParts.cpp parts.cpp stage.cpp RocketSolver.cpp NelderMead.cpp)
target_include_directories(debug PRIVATE ext)
target_compile_options(debug PRIVATE -no-pie -g -O0 -fno-omit-frame-pointer -Wall -Wunused-value -Wshadow -pedantic)
target_link_options(debug PRIVATE -no-pie)

add_executable(release main.cpp rocket.cpp LoadKspParts.cpp parts.cpp stage.cpp RocketSolver.cpp NelderMead.cpp)
target_include_directories(release PRIVATE ext)
target_compile_options(release PRIVATE -Wall -Wunused-value -Wshadow -pedantic -O3 -DNDEBUG)
