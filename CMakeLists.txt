cmake_minimum_required(VERSION 3.14)

project(rocket)

set(CMAKE_CXX_STANDARD 20)

add_executable(debug main.cpp rocket.cpp LoadKspParts.cpp parts.cpp stage.cpp RocketSolver.cpp NelderMead.cpp)
target_include_directories(debug PRIVATE ext)
target_compile_options(debug PRIVATE -no-pie -g -O0 -fno-omit-frame-pointer -Wall -Wunused-value -Wshadow -pedantic)
target_link_options(debug PRIVATE -no-pie)

add_executable(release main.cpp rocket.cpp LoadKspParts.cpp parts.cpp stage.cpp RocketSolver.cpp NelderMead.cpp)
target_include_directories(release PRIVATE ext)
target_compile_options(release PRIVATE -Wall -Wunused-value -Wshadow -pedantic -O3 -DNDEBUG)

# ---- GoogleTest ----
find_package(GTest REQUIRED)

add_executable(unittest unittest.cpp rocket.cpp parts.cpp stage.cpp LoadKspParts.cpp NelderMead.cpp RocketSolver.cpp)
target_include_directories(unittest PRIVATE ext)
target_compile_options(unittest PRIVATE -g -O0 -Wall -Wunused-value -Wshadow -pedantic)
target_link_libraries(unittest GTest::gtest_main)
add_test(NAME unittest COMMAND unittest)
