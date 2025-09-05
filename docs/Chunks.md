### Chunks

> [!NOTE]  
> Confused why and what are "voxels" doing in a 2D game engine? Please refer to the note in `Engine.md`

Chunks work in a similar way as the popular hit game *Minecraft*. Each chunk holds a 2D array of voxels. The chunk is responsible for managing its voxels

They have their own Box2D meshes, manage their own GPU simulation and rendering data etc.

A lot of methods are called directly by the `ChunkMatrix` managing them. You shouldn't have to mess with them too much out of the box.

### ChunkMatrix

ChunkMatrix is a class build for managing and interacting with chunks in a simple way. Its one of the most important classes in the whole engine. It could be compared to a world or a level in other engines. To simplify working with individual voxels in the game, you should use `ChunkMatrix::VirtualGetAt` and `ChunkMatrix::VirtualSetAt` to interact with voxels individually. They are called *Virtual...* since they act like if you were accesing a 2D array that hosts all the voxels in the ChunkMatrix, even when its not. They also have a *_NoLoad* and *_NoDelete* variation which are used for more fine tuned control. 

NoLoad means that it won't load any new chunks by itself. If a chunk at that place isn't loaded you just get an empty pointer. 

NoDelete is a bit more dangerous. You override the voxel at that position, replacing it with the new one **but** the pointer of the previous voxel **is not** deleted. Any reference to that pointer is still valid until deleted by **you** or until its given back in a ChunkMatrix's control

> [!IMPORTANT]  
> Use `ChunkMatrix::VirtualSetAt_NoDelete` with caution. Failing to do so **will** generate memory leaks in your application which **will** pile up in an unexpectedly short time. 

Currently the ChunkMatrix is only a single class inside the `GameEngine` object. In the future (or currently, if I forgot to update this doc) I will want to make the ChunkMatrix a pointer which will allow swapping them during runtime. It would be like loading a new level or world in any other engine.