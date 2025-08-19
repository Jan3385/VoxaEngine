cmake_minimum_required(VERSION 3.10)
project(Voxa LANGUAGES C CXX)

# vcpkg dependencies
find_package(SDL2 CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Poly2Tri (not in vcpkg)
include(FetchContent)
FetchContent_Declare(
  poly2tri
  GIT_REPOSITORY https://github.com/greenm01/poly2tri.git
  GIT_TAG master
)
FetchContent_MakeAvailable(poly2tri)
add_library(poly2tri STATIC
  ${poly2tri_SOURCE_DIR}/poly2tri/common/shapes.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/advancing_front.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/cdt.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/sweep.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/sweep_context.cc
)
target_include_directories(poly2tri PUBLIC ${poly2tri_SOURCE_DIR})

# ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.0
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

if(TARGET SDL2::SDL2-static)
  target_link_libraries(imgui PUBLIC SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
  target_link_libraries(imgui PUBLIC SDL2::SDL2)
endif()

message(STATUS "Dependencies prepared")

if(UNIX)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Subprojects
add_subdirectory(Engine)

# Select which subprojects to build
set(PROJECT "Game" CACHE STRING "Select the project to build (eg. -DPROJECT=Game)")
add_subdirectory(Games/${PROJECT})