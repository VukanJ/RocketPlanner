cmake_minimum_required(VERSION 3.14)

project(rocket)

set(CMAKE_CXX_STANDARD 23)

set(ROCKET_SOURCES
  src/main.cpp
  src/rocket.cpp
  src/LoadKspParts.cpp
  src/parts.cpp
  src/PartSerializer.cpp
  src/stage.cpp
  src/RocketSolver.cpp
  src/NelderMead.cpp
  src/cmdargs.cpp
  src/WindowSimulator.cpp
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

add_executable(debug ${ROCKET_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES})
target_include_directories(debug PRIVATE ext src ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${implot_SOURCE_DIR} /usr/include)
target_compile_definitions(debug PRIVATE AGAVE_FONT_PATH="${AGAVE_FONT_PATH}")
target_compile_options(debug PRIVATE -no-pie -g -O0 -fno-omit-frame-pointer -Wall -Wunused-value -Wshadow -pedantic)
target_link_options(debug PRIVATE -no-pie)
target_link_libraries(debug glfw OpenGL::GL)

add_executable(release ${ROCKET_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES})
target_include_directories(release PRIVATE ext src ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${implot_SOURCE_DIR} /usr/include)
target_compile_definitions(release PRIVATE AGAVE_FONT_PATH="${AGAVE_FONT_PATH}")
target_compile_options(release PRIVATE -Wall -Wunused-value -Wshadow -pedantic -O3 -DNDEBUG)
target_link_libraries(release glfw OpenGL::GL)

# ---- GoogleTest ----
find_package(GTest REQUIRED)

add_executable(unittest src/unittest.cpp src/rocket.cpp src/parts.cpp src/PartSerializer.cpp src/stage.cpp src/LoadKspParts.cpp src/NelderMead.cpp src/RocketSolver.cpp)
target_include_directories(unittest PRIVATE ext src /usr/include)
target_compile_options(unittest PRIVATE -g -O0 -Wall -Wunused-value -Wshadow -pedantic)
target_link_libraries(unittest GTest::gtest_main)
add_test(NAME unittest COMMAND unittest)


