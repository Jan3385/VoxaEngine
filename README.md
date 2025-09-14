# **VoxaEngine**

![Game Image](/Promotional-stuff/banner.png)

> #### **Disclaimer**: The definiton of a voxel was slightly bended when making this engine. A voxel here represents a set 3D **OR 2D** volume as a square/cube which is generally incorrect but so is pixel and calling them squares doesn't sound nice. 

VoxaEngine is currently capable of making sand falling simulations, fire simulations, heat transfer & pressure simulations.

Uses **SDL2**, **FreeType** library and **Dear ImGui**. Engine is programmed in **C++** and rendered using **OpenGL**.

Features *OpenGL* compute shaders using **GLEW** for faster simulation passes.

### Sand Falling simulation

![Sand simulation Gif](/Promotional-stuff/sand-simulation.gif)

Utilizes Dirty Rect's and multithreading for maximum performance.

It is capable of simulating solids, liquids and gasses all interacting with each other.

Each voxel has a density, which affects the outcome of the simulation.

### Heat Transfer simulation

Capable of trasfering heat between voxels and changing voxel states based on temperature.

Utilizes multithreading and sectioning for extra performance.

Most of the heat transfer is done on the GPU for extra performace

Simulation includes heat capacity and heat conductivity for each voxel for more realistic transfers.

### Pressure Transfer simulation

Capable of simulating gas and liquid "pressures" and their ability to compress or stretch in avalible space.

Most of the gas pressure simulation is done on the GPU for faster speeds

### Fire simulation

![Fire GIF](/Promotional-stuff/fire.gif)

Utilizing flamability and flame propagation with oxygen

### Physics simulation

![Simulation GIF](/Promotional-stuff/physics-simulation.gif)

Utilizing Box2D and a custom object logic to support fully destructable, dynamic objects

### OpenGL rendering

SDL2 renderer and most of other renderers are not made with this kind of "voxel" rendering in mind. This is why this project runs on custom render shaders to provide maximum performance that wouldn't be possible without it

# How To Run:

> [!NOTE]  
> ImGui and poly2tri libraries are dynamically downloaded on build. Your first build may take few minutes

> [!CAUTION]
> Installing and setting up `vcpkg` is an extensive process. You will have to install your OSs triplets and such. If you have no experience with `vcpkg` it's recommended to first look up a tutorial to set it up correctly before compiling this project

> [!TIP]
> There are prebuild binaries avalible which you can just download and immediately run

## Windows 10/11

> [!IMPORTANT]
> You need to have installed `CMake` v3.10 or more, `g++` compiler with `Ninja` generator.
> You will need to have `vcpkg` installed somewhere to use its path to get *vcpkg.cmake*

(All commands are inside the *PROJECT* folder)

To prepare the project for compilation run:

`cmake -S . -B build`

> [!NOTE]
> If you do not have **VCPKG_ROOT** enviroment variable configured use the following command instead:
> 
> `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="PATH\TO\VCPKG\CMAKE\vcpkg.cmake"`

Afterwards run following to set the release preset:

`cmake --preset release` 

Build the project into executable binaries with:

`cmake --build build` 

After that, run `VoxaEngine.exe` from **inside** the *build/Games/Game* folder.

## Linux

> [!IMPORTANT]
> You need to have installed `CMake` v3.10 or more, `g++` compiler with `Ninja` generator.
> You will need to have `vcpkg` installed somewhere to use its path to get *vcpkg.cmake*

You need to install these packages before compiling (package name depends on your linux distribution):
- `libglew-dev` or `glew`
- `libgl1-mesa-dev` & `libgl-dev` or `mesa`


(All commands are inside the *PROJECT* folder)

To prepare the project for compilation run:

`cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="PATH/TO/VCPKG/CMAKE/vcpkg.cmake"`

Afterwards run following to set the release preset:

`cmake --preset release` 

Build the project into executable binaries with:

`cmake --build build` 

After that, run `VoxaEngine` from **inside** the *build/Games/Game* folder.

# Controls

 - `w` `a` `s` `d` - movement (`w` and `s` - swim up and down or jump when not using noclip)
 - `left click` - place or shoot
 - `right click` - explode
 - `t` `z` `u` - place a physics barrel, physics sphere, static crate
