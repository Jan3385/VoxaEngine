set(CMAKE_GENERATOR "Ninja")

cmake_minimum_required(VERSION 3.10)

# Set compiler
set(CMAKE_CXX_COMPILER "g++")

project(VoxaEngine VERSION 0.2.0 LANGUAGES CXX)

# SDL2 paths
set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/SDL2/include/SDL2")

set(IMGUI_INCLUDE_HEADER_DIR "${CMAKE_SOURCE_DIR}/SDL2/include/imgui")
set(IMGUI_INCLUDE_CPP_DIR "${CMAKE_SOURCE_DIR}/imgui")

set(SDL2_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2.dll.a")
set(SDL2_MAIN_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2main.a")
set(SDL2_DLL "${CMAKE_SOURCE_DIR}/SDL2/SDL2.dll")

set(GLEW_LIBRARY "${CMAKE_SOURCE_DIR}/GLEW/lib/Release/x64/glew32.lib")
set(GLEW_INCLUDE "${CMAKE_SOURCE_DIR}/GLEW/include/GL")
set(GLEW_DLL "${CMAKE_SOURCE_DIR}/GLEW/glew32.dll")

set(SDL2_LIBRARY_TFF "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2_ttf.dll.a")
set(SDL2_TTF_DLL "${CMAKE_SOURCE_DIR}/SDL2/SDL2_ttf.dll")

set(FONTS_FOLDER "${CMAKE_SOURCE_DIR}/Fonts")
set(TEXTURES_FOLDER "${CMAKE_SOURCE_DIR}/Textures")

# Add include directories for SDL2
include_directories(${SDL2_INCLUDE_DIR} ${IMGUI_INCLUDE_HEADER_DIR} ${GLEW_INCLUDE})

# Automatically include all .cpp files in the Files folder
file(GLOB_RECURSE SOURCES "Files/*.cpp")
file(GLOB_RECURSE IMGUI_SOURCES "${IMGUI_INCLUDE_CPP_DIR}/*.cpp")

add_executable(VoxaEngine ${SOURCES} ${IMGUI_SOURCES})

find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(VoxaEngine OpenMP::OpenMP_CXX)
endif()

# Link SDL2 libraries
target_link_libraries(VoxaEngine ${SDL2_LIBRARY} ${SDL2_MAIN_LIBRARY} ${SDL2_LIBRARY_TFF} ${GLEW_LIBRARY})

set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 20)

# set_target_properties(VoxaEngine PROPERTIES LINK_FLAGS "-mwindows")

add_custom_command(TARGET VoxaEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_DLL} $<TARGET_FILE_DIR:VoxaEngine>
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_TTF_DLL} $<TARGET_FILE_DIR:VoxaEngine>

    COMMAND ${CMAKE_COMMAND} -E copy ${GLEW_DLL} $<TARGET_FILE_DIR:VoxaEngine>

    COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Fonts
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${FONTS_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Fonts
    
    COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:VoxaEngine>/Textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURES_FOLDER} $<TARGET_FILE_DIR:VoxaEngine>/Textures
)