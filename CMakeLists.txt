cmake_minimum_required(VERSION 3.14)

project(rocket)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(ROCKET_SOURCES
  src/main.cpp
  src/rocket.cpp
  src/LoadKspParts.cpp
  src/parts.cpp
  src/Orbit.cpp
  src/PartSerializer.cpp
  src/stage.cpp
  src/RocketSolver.cpp
  src/NelderMead.cpp
  src/cmdargs.cpp
  src/WindowSimulator.cpp
  src/DeltaV.cpp
  src/RocketConfig.cpp
  src/FlightSimulator.cpp
  src/gui.cpp
  src/WindowMission.cpp
  src/SystemMap.cpp
  src/LambertSolver.cpp
  src/ThreadPool.cpp
)

# ---- Agave Font ----
set(AGAVE_FONT_URL "https://raw.githubusercontent.com/blobject/agave/master/dist/Agave-Regular.ttf")
set(AGAVE_FONT_PATH "${CMAKE_BINARY_DIR}/Agave-Regular.ttf")
if(NOT EXISTS ${AGAVE_FONT_PATH})
  message(STATUS "Downloading Agave font...")
  file(DOWNLOAD ${AGAVE_FONT_URL} ${AGAVE_FONT_PATH} STATUS dl_status)
  list(GET dl_status 0 dl_code)
  list(GET dl_status 1 dl_msg)
  if(dl_code EQUAL 0)
    message(STATUS "Agave font downloaded")
  else()
    message(WARNING "Failed to download Agave font: ${dl_msg}")
  endif()
endif()

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

# ---- ImPlot ----
FetchContent_Declare(
  implot
  GIT_REPOSITORY https://github.com/epezent/implot.git
  GIT_TAG v0.16
)
FetchContent_MakeAvailable(implot)

set(IMPLOT_SOURCES
  ${implot_SOURCE_DIR}/implot.cpp
  ${implot_SOURCE_DIR}/implot_items.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(TARGET_NAME release)
else()
  set(TARGET_NAME debug)
endif()

add_executable(${TARGET_NAME} ${ROCKET_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ext src ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${implot_SOURCE_DIR} /usr/include /usr/include/eigen3)
target_compile_definitions(${TARGET_NAME} PRIVATE AGAVE_FONT_PATH="${AGAVE_FONT_PATH}")
target_compile_options(${TARGET_NAME} PRIVATE -no-pie -fno-omit-frame-pointer -march=native -Wall -Wunused-value -Wshadow -pedantic)
target_link_options(${TARGET_NAME} PRIVATE -no-pie)
target_link_libraries(${TARGET_NAME} glfw OpenGL::GL)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(${TARGET_NAME} PRIVATE -O3 -DNDEBUG)
else()
  target_compile_options(${TARGET_NAME} PRIVATE -O0 -g)
endif()

# ---- GoogleTest ----
find_package(GTest REQUIRED)

set(UNITTEST_SOURCES
  src/unittest.cpp src/rocket.cpp src/parts.cpp src/PartSerializer.cpp
  src/stage.cpp src/LoadKspParts.cpp src/NelderMead.cpp
  src/RocketSolver.cpp src/RocketConfig.cpp src/DeltaV.cpp src/Orbit.cpp
  src/LambertSolver.cpp
)

add_executable(unittest ${UNITTEST_SOURCES})
target_include_directories(unittest PRIVATE ext src /usr/include /usr/include/eigen3)
target_compile_options(unittest PRIVATE -march=native -Wall -Wunused-value -Wshadow -pedantic -g -O0)
target_link_libraries(unittest GTest::gtest_main)
add_test(NAME unittest COMMAND unittest)
