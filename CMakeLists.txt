set(CMAKE_GENERATOR "Ninja")

cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

message(STATUS "Using CMake generator: ${CMAKE_GENERATOR}")

project(VoxaEngine VERSION 0.2.1 LANGUAGES C CXX)

set(FONTS_FOLDER "${CMAKE_SOURCE_DIR}/Fonts")
set(TEXTURES_FOLDER "${CMAKE_SOURCE_DIR}/Textures")

message(STATUS "Fetching dependencies...")

include(FetchContent)

# TODO: Add vcpkg support

# SDL2
FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.32.2
)
FetchContent_MakeAvailable(SDL2)

# Box2D
FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(box2d)

# poly2tri (mesh triangulation)
FetchContent_Declare(
  poly2tri
  GIT_REPOSITORY https://github.com/greenm01/poly2tri.git
  GIT_TAG master
)
FetchContent_MakeAvailable(poly2tri)

# Add poly2tri as a static library (since it doesn't provide a CMake target)
add_library(poly2tri STATIC
  ${poly2tri_SOURCE_DIR}/poly2tri/common/shapes.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/advancing_front.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/cdt.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/sweep.cc
  ${poly2tri_SOURCE_DIR}/poly2tri/sweep/sweep_context.cc
)

target_include_directories(poly2tri PUBLIC
  ${poly2tri_SOURCE_DIR}
)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# FreeType
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://github.com/freetype/freetype.git
  GIT_TAG VER-2-13-3
)
FetchContent_MakeAvailable(freetype)

# ImGui (no native CMake build)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.0
)
FetchContent_MakeAvailable(imgui)

message(STATUS "Dependencies fetched")

# Add ImGui manually as a static library
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

# Link SDL2 to imgui so SDL2 headers are available for imgui_impl_sdl2.cpp
if(TARGET SDL2-static)
  target_link_libraries(imgui PUBLIC SDL2-static)
elseif(TARGET SDL2)
  target_link_libraries(imgui PUBLIC SDL2)
endif()

message(STATUS "ImGui library created")

# Automatically include all .cpp files in the Files folder
file(GLOB_RECURSE SOURCES "Files/*.cpp")

add_executable(VoxaEngine ${SOURCES})

# OpenMP support (pragma omp)
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
  target_link_libraries(VoxaEngine PRIVATE OpenMP::OpenMP_CXX)
endif()

# check OpenGL
find_package(OpenGL REQUIRED)

# Link all FetchContent dependencies
# SDL2 target name check (static or shared)
if(TARGET SDL2-static)
  target_link_libraries(VoxaEngine PRIVATE SDL2-static)
elseif(TARGET SDL2)
  target_link_libraries(VoxaEngine PRIVATE SDL2)
endif()

message(STATUS "Linking platform-specific libraries...")

# Platform-specific GLEW linking
if(WIN32)
  target_link_libraries(VoxaEngine PRIVATE
    imgui
    box2d
    poly2tri
    glm
    freetype
    ${PROJECT_SOURCE_DIR}/GLEW/lib/Release/x64/glew32s.lib
    OpenGL::GL
  )

  # Link local GLEW static library
  target_include_directories(VoxaEngine PRIVATE ${PROJECT_SOURCE_DIR}/GLEW/include)
  add_definitions(-DGLEW_STATIC)
elseif(UNIX)
  find_package(GLEW REQUIRED)
  target_link_libraries(VoxaEngine PRIVATE
    imgui
    box2d
    poly2tri
    glm
    freetype
    GLEW::GLEW
    OpenGL::GL
  )
endif()

message(STATUS "Linking libraries completed")

# SDL main function override
add_definitions(-DSDL_MAIN_HANDLED)

# Include project Files folder
target_include_directories(VoxaEngine PRIVATE ${PROJECT_SOURCE_DIR}/Files)

# Add include directories for fetched libraries
target_include_directories(VoxaEngine PRIVATE
  ${box2d_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${freetype_SOURCE_DIR}/include
)


# Set C++ standard
set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 20)

message(STATUS "Running post-build commands...")

# Post build: copy Fonts and Textures
add_custom_command(TARGET VoxaEngine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Fonts
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Fonts
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Textures
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURES_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Textures
)
