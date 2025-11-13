if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE is not defined. Make sure you specify it with DCMAKE_TOOLCHAIN_FILE")
endif()

project(VoxaEngine VERSION 0.2.1 LANGUAGES C CXX)

# vcpkg dependencies
find_package(SDL2 CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

message(STATUS "Fetching libraries (this might take a minute)")

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

# Collect sources
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(VoxaEngine STATIC ${ENGINE_SOURCES})

# Engine include path
target_include_directories(VoxaEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# SDL main override
target_compile_definitions(VoxaEngine PUBLIC SDL_MAIN_HANDLED)

# OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
  target_link_libraries(VoxaEngine PRIVATE OpenMP::OpenMP_CXX)
endif()

# Platform specifics
if(WIN32)
  target_link_libraries(VoxaEngine
    PUBLIC
      SDL2::SDL2
      box2d::box2d
      poly2tri
      glm::glm
      Freetype::Freetype
      imgui
      ${CMAKE_CURRENT_SOURCE_DIR}/GLEW/lib/Release/x64/glew32s.lib
      OpenGL::GL
  )
  target_include_directories(VoxaEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/GLEW/include)
  target_compile_definitions(VoxaEngine PUBLIC GLEW_STATIC)
elseif(UNIX)
  find_package(GLEW REQUIRED)
  target_link_libraries(VoxaEngine
    PUBLIC
      SDL2::SDL2
      box2d::box2d
      poly2tri
      glm::glm
      Freetype::Freetype
      imgui
      GLEW::GLEW
      OpenGL::GL
  )
endif()

set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 20)