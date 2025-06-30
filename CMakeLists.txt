set(CMAKE_GENERATOR "Ninja")

cmake_minimum_required(VERSION 3.10)

# Set compiler
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(VoxaEngine VERSION 0.2.1 LANGUAGES C CXX)

# SDL2 paths
set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/SDL2/include/SDL2")
set(SDL2_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2.dll.a")
set(SDL2_MAIN_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2main.a")
set(SDL2_DLL "${CMAKE_SOURCE_DIR}/SDL2/SDL2.dll")

set(IMGUI_INCLUDE_HEADER_DIR "${CMAKE_SOURCE_DIR}/SDL2/include/imgui")
set(IMGUI_INCLUDE_CPP_DIR "${CMAKE_SOURCE_DIR}/imgui")


set(GLEW_INCLUDE "${CMAKE_SOURCE_DIR}/GLEW/include/GL")
set(GLEW_DLL "${CMAKE_SOURCE_DIR}/GLEW/glew32.dll")


set(FONTS_FOLDER "${CMAKE_SOURCE_DIR}/Fonts")
set(TEXTURES_FOLDER "${CMAKE_SOURCE_DIR}/Textures")

# Box2D integration
set(BOX2D_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/box2D/include")
file(GLOB_RECURSE BOX2D_SOURCES "${CMAKE_SOURCE_DIR}/box2D/src/*.c")

set_source_files_properties(${BOX2D_SOURCES} PROPERTIES LANGUAGE C)

# Add include directories for SDL2
include_directories(${SDL2_INCLUDE_DIR} ${IMGUI_INCLUDE_HEADER_DIR} ${GLEW_INCLUDE} ${BOX2D_INCLUDE_DIR})

include(FetchContent)

# Add GLM (OpenGL Mathematics) library
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Add FreeType library
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-2
)
FetchContent_MakeAvailable(freetype)

# Automatically include all .cpp files in the Files folder
file(GLOB_RECURSE SOURCES "Files/*.cpp")
file(GLOB_RECURSE IMGUI_SOURCES "${IMGUI_INCLUDE_CPP_DIR}/*.cpp")

add_executable(VoxaEngine ${SOURCES} ${IMGUI_SOURCES} ${BOX2D_SOURCES})

# OpenMP support (pragma omp)
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(VoxaEngine PRIVATE OpenMP::OpenMP_CXX)
endif()

# Link GLEW library
find_package(OpenGL REQUIRED)
if(WIN32)
    set(GLEW_LIBRARY "${CMAKE_SOURCE_DIR}/GLEW/lib/Release/x64/glew32.lib")
    target_link_libraries(VoxaEngine PRIVATE ${GLEW_LIBRARY} OpenGL::GL)
elseif(UNIX)
    find_package(GLEW REQUIRED)
    target_link_libraries(VoxaEngine PRIVATE ${GLEW_LIBRARIES} OpenGL::GL)
endif()

# Link SDL2 libraries
if (WIN32)
    set(SDL2_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2.dll.a")
    set(SDL2_MAIN_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2main.a")
    target_link_libraries(VoxaEngine PRIVATE ${SDL2_LIBRARY} ${SDL2_MAIN_LIBRARY})
elseif (UNIX)
    find_package(SDL2 REQUIRED)
    target_include_directories(VoxaEngine PRIVATE ${SDL2_INCLUDE_DIRS})
endif()

# Link GLM library
target_link_libraries(VoxaEngine PRIVATE glm)
message(STATUS "GLM source dir: ${glm_SOURCE_DIR}")

# Link FreeType library
target_link_libraries(VoxaEngine PRIVATE freetype)

# Link project libraries
target_include_directories(VoxaEngine PRIVATE ${PROJECT_SOURCE_DIR}/Files)

set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 20)

add_custom_command(TARGET VoxaEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_DLL} $<TARGET_FILE_DIR:VoxaEngine>

    COMMAND ${CMAKE_COMMAND} -E copy ${GLEW_DLL} $<TARGET_FILE_DIR:VoxaEngine>

    COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Fonts
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Fonts
    
    COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURES_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Textures
)