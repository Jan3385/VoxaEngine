> [!NOTE]  
> During the making of this engine I have decited to call the squares that are used in the celluar automata and the main driving point of this engine `voxels`. I am aware that these squares do *not* fall into that definition since voxel is a shorthand for volumetric pixel. I am missing an extra dimension for the *voxels* to be volumetric. I havent found any other good definition for the squares in the engine as pixel doesnt entirely fit that definition too and I am just not calling them squares everywhere. So anytime you see the word "voxel" just pretend with me that it can and does mean a 2D square *or* 3D cube.

The entire engine code is inside of the `Engine/` folder. I am planning to always keep the engine open source forever

The game engine works as following:
- The GameEngine class holds all the necesary data, along with voxel and object registries and such
- At the start, you register your objects and voxels to the registies and configure the chunk spawning logic
- The GameEngine uses the ChunkMatrix inside to run the world and GPU simulations
- GameEngine handles the game loop, you are given access to methods from `IGame`