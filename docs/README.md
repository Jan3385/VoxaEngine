> [!NOTE]  
> During the making of this engine I have decided to call the squares that are used in the cellular automata and the main driving point of this engine `voxels`. I am aware that these squares do *not* fall into that definition since voxel is a shorthand for volumetric pixel. I am missing an extra dimension for the *voxels* to be volumetric. I haven't found any other good definition for the squares in the engine as pixel doesn't entirely fit that definition too and I am just not calling them squares everywhere. So anytime you see the word "voxel" just pretend with me that it can and does mean a 2D square *or* 3D cube.

This engine is currently just a prototype. I do not promise any compatibility between older and newer versions

The entire engine code is inside of the `Engine/` folder. I am planning to always keep the engine open source forever

The game engine works as follows:
- The GameEngine class holds all the necessary data, along with voxel and object registries and such
- At the start, you register your objects and voxels to the registries and configure the chunk spawning logic
- The GameEngine uses the ChunkMatrix inside to run the world and GPU simulations
- GameEngine handles the game loop, you are given access to methods from `IGame`