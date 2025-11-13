cmake_minimum_required(VERSION 3.10)

if(WIN32)
  set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "" FORCE)
  set(VCPKG_HOST_TRIPLET "x64-mingw-static" CACHE STRING "" FORCE)
elseif(UNIX)
  set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "" FORCE)
  set(VCPKG_HOST_TRIPLET "x64-linux" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Voxa LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VCPKG_HOS)

# Subprojects
add_subdirectory(Engine)

# Select which subprojects to build
set(PROJECT "Game" CACHE STRING "Select the project to build (eg. -DPROJECT=Game)")
add_subdirectory(Apps/${PROJECT})