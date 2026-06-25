cmake_minimum_required(VERSION 3.14)

project(rocket)

set(CMAKE_CXX_STANDARD 23)

set(ROCKET_SOURCES
  src/main.cpp
  src/rocket.cpp
  src/LoadKspParts.cpp
  src/parts.cpp
  src/stage.cpp
  src/RocketSolver.cpp
  src/NelderMead.cpp
  src/cmdargs.cpp
)

# ---- ImGui + GLFW ----
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.8
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(debug ${ROCKET_SOURCES} ${IMGUI_SOURCES})
target_include_directories(debug PRIVATE ext src ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_options(debug PRIVATE -no-pie -g -O0 -fno-omit-frame-pointer -Wall -Wunused-value -Wshadow -pedantic)
target_link_options(debug PRIVATE -no-pie)
target_link_libraries(debug glfw OpenGL::GL)

add_executable(release ${ROCKET_SOURCES} ${IMGUI_SOURCES})
target_include_directories(release PRIVATE ext src ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_options(release PRIVATE -Wall -Wunused-value -Wshadow -pedantic -O3 -DNDEBUG)
target_link_libraries(release glfw OpenGL::GL)

# ---- GoogleTest ----
find_package(GTest REQUIRED)

add_executable(unittest src/unittest.cpp src/rocket.cpp src/parts.cpp src/stage.cpp src/LoadKspParts.cpp src/NelderMead.cpp src/RocketSolver.cpp)
target_include_directories(unittest PRIVATE ext src)
target_compile_options(unittest PRIVATE -g -O0 -Wall -Wunused-value -Wshadow -pedantic)
target_link_libraries(unittest GTest::gtest_main)
add_test(NAME unittest COMMAND unittest)


