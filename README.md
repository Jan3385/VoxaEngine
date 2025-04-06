# **VoxaEngine**
> a small sand falling engine build with SDL2 and ImGui in C++

VoxaEngine is currently capable of making sand falling simulations, heat transfer & pressure simulations.

Uses **SDL2** with **SDL_TTF** library and **Dear ImGui**. Engine is entirely programmed in C++.

Features basic OpenGL compute shaders using **GLEW** for faster computing.

### Sand Falling simulation
Utilizes Dirty Rect's and multithreading for maximum performance.

It is capable of simulating solids, liquids and gasses all interacting with each other.

Each voxel has a density that changes the simulation's outcome.

### Heat Transfer simulation
Capable of trasfering heat between voxels and changing voxel states based on temperature.

Utilizes multithreading, sectioning and flagging chunks needing to compute heat for extra performance.

Parts of the heat transfer is done on the GPU for extra performace

Simulation includes heat capacity and heat conductivity for each voxel for more realistic transfers.

### Pressure Transfer simulation
Capable of simulating gas and liquid "pressures" and is able to compress or stretch those fluids in avalible space.

A small part of the pressure simulation is done on the GPU for faster speeds

# How To Run:

## Windows 10/11

Make sure you have at least *CMake v3.10* installed and functional. Ensure you have a *g++* compiler with *Ninja* generator installed and working with CMake. 

To compile simply run `cmake --preset release` and follow with `cmake --build build` in the project folder.

After that, run `VoxaEngine.exe` inside the build folder.

## UNIX Linux

Make sure you have at least *CMake v3.10* installed and functional. Ensure you have a *g++* compiler with *Ninja* generator installed and working with CMake. 

You need to install these packages before compiling:
- `glew` or `libglew-dev`
- `sdl2`
- `sdl2_ttf`

To compile simply run `cmake --preset release` and follow with `cmake --build build` in the project folder.

After that, run `VoxaEngine` inside the build folder.