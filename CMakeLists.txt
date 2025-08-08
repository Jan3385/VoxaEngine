
cmake_minimum_required(VERSION 3.10)

project(VoxaEngine VERSION 0.2.1 LANGUAGES C CXX)

set(FONTS_FOLDER "${CMAKE_SOURCE_DIR}/Fonts")
set(TEXTURES_FOLDER "${CMAKE_SOURCE_DIR}/Textures")

# Use vcpkg for dependencies
find_package(SDL2 CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

# poly2tri (mesh triangulation) - not in vcpkg
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
if(TARGET SDL2::SDL2-static)
  target_link_libraries(imgui PUBLIC SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
  target_link_libraries(imgui PUBLIC SDL2::SDL2)
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

if (MINGW)
  target_link_options(VoxaEngine PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
  )
endif()

# check OpenGL
find_package(OpenGL REQUIRED)

# SDL2 main function override
add_definitions(-DSDL_MAIN_HANDLED)

# Include project Files folder and ImGui backends
target_include_directories(VoxaEngine PRIVATE ${PROJECT_SOURCE_DIR}/Files)

# Platform-specific GLEW linking
if(WIN32)
  target_link_libraries(VoxaEngine PRIVATE
    SDL2::SDL2
    $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main>
    box2d::box2d
    poly2tri
    glm::glm
    Freetype::Freetype
    imgui
    ${PROJECT_SOURCE_DIR}/GLEW/lib/Release/x64/glew32s.lib
    OpenGL::GL
  )
  target_include_directories(VoxaEngine PRIVATE ${PROJECT_SOURCE_DIR}/GLEW/include)
  add_definitions(-DGLEW_STATIC)
elseif(UNIX)
  find_package(GLEW REQUIRED)
  target_link_libraries(VoxaEngine PRIVATE
    SDL2::SDL2
    $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main>
    box2d::box2d
    poly2tri
    glm::glm
    Freetype::Freetype
    imgui
    GLEW::GLEW
    OpenGL::GL
  )
endif()

# Set C++ standard
set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 20)

# Post build: copy Fonts and Textures
add_custom_command(TARGET VoxaEngine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Fonts
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Fonts
  COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Textures
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURES_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Textures
)
