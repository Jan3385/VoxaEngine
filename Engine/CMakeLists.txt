project(VoxaEngine VERSION 0.2.1 LANGUAGES C CXX)

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
      $<$<TARGET_EXISTS:SDL2::SDL2main>:SDL2::SDL2main>
      box2d::box2d
      poly2tri
      glm::glm
      Freetype::Freetype
      imgui
      ${CMAKE_SOURCE_DIR}/GLEW/lib/Release/x64/glew32s.lib
      OpenGL::GL
  )
  target_include_directories(VoxaEngine PUBLIC ${CMAKE_SOURCE_DIR}/GLEW/include)
  target_compile_definitions(VoxaEngine PUBLIC GLEW_STATIC)
elseif(UNIX)
  find_package(GLEW REQUIRED)
  target_link_libraries(VoxaEngine
    PUBLIC
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

set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 20)