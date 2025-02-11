# **VoxaEngine**
> a small sand falling engine build with SDL2 and ImGui in C++

VoxaEngine is currently capable of making sand falling simulations and heat transfer simulations

Uses **SDL2** with **SDL_TTF** library and **Dear ImGui**. Engine is entirely programmed in C++.

### Sand Falling simulation
Utilizes Dirty Rect's and multithreading for maximum performance.

It is capable of simulating solids, liquids and gasses all interacting with each other.

Each voxel has a density that changes the simulation's outcome.

### Heat Transfer simulation
Capable of trasfering heat between voxels and changing voxel states based on temperature.

Utilizes multithreading, sectioning and flagging chunks needing to compute heat for extra performance.

Simulation includes heat capacity and heat conductivity for each voxel for more realistic transfers.

### How To Run:

Make sure you have at least CMake v3.10 installed and functional. Ensure you have a g++ compiler installed and working with CMake. 

To compile simply run `cmake --preset release` and follow with `cmake --build build` in the project folder.

After that, run `VoxaEngine.exe` inside the build folder.

*Currently, only Windows is supported!*