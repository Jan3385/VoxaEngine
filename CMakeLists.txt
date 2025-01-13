cmake_minimum_required(VERSION 3.5)
project(VoxaEngine VERSION 0.1.0 LANGUAGES CXX)

# SDL2 paths
set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/SDL2/include")
set(SDL2_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2.dll.a")
set(SDL2_MAIN_LIBRARY "${CMAKE_SOURCE_DIR}/SDL2/lib/libSDL2main.a")
set(SDL2_DLL "${CMAKE_SOURCE_DIR}/SDL2/SDL2.dll")

# Add include directories for SDL2
include_directories(${SDL2_INCLUDE_DIR})

# Automatically include all .cpp files in the Files folder
file(GLOB_RECURSE SOURCES "Files/*.cpp")

add_executable(VoxaEngine ${SOURCES})

# Link SDL2 libraries
target_link_libraries(VoxaEngine ${SDL2_LIBRARY} ${SDL2_MAIN_LIBRARY})

set_property(TARGET VoxaEngine PROPERTY CXX_STANDARD 17)

# set_target_properties(VoxaEngine PROPERTIES LINK_FLAGS "-mwindows")

# Copy SDL2.dll to the output directory
add_custom_command(TARGET VoxaEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_DLL} $<TARGET_FILE_DIR:VoxaEngine>
)