cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "g++")

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Voxa LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Subprojects
add_subdirectory(Engine)

# Select which subprojects to build
set(PROJECT "Game" CACHE STRING "Select the project to build (eg. -DPROJECT=Game)")
add_subdirectory(Apps/${PROJECT})