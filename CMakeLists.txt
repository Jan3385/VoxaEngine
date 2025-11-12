cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Voxa LANGUAGES C CXX)

# Subprojects
add_subdirectory(Engine)

# Select which subprojects to build
set(PROJECT "Game" CACHE STRING "Select the project to build (eg. -DPROJECT=Game)")
add_subdirectory(Apps/${PROJECT})