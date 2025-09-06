
[README]: README.md

> [!NOTE]  
> Confused about why and what "voxels" are doing in a 2D game engine? Please refer to the note in [README.md][README]

# Voxels

> NAMESPACE: Volume

## Buildin voxel types

> BASE_CLASS: Volume::VoxelElement

<img src="images/voxels.gif" alt="Voxels interacting" title="Showcase of the buildin voxel types interacting" width="440">

The provided buildin voxel types are a basic selection of voxels made for simulations reminding of real life. If the selection does not fit your usecase you are encouraged to expand the list in your own application by inhereting from the `Volume::VoxelElement` base class

The most important method is the `bool Volume::VoxelElement::Step(ChunkMatrix* matrix)` which may be called each voxel update. It must be overriden to implement any custom functionality like movement. It should return `true` if you expect the element to be also updated the next frame (eg. if it moved this frame). Returning `false` does ***not** guarantee it won't be called the again next or any other frame. You should also change the `bool Volume::VoxelElement::updatedThisFrame` to `true` to prevent the voxel to be updated more than once per frame

Basic implementation of a `MyVoxel` class. The following example will make a voxel that falls straight down until it encounters something solid. After that it deletes itself
```cpp
class MyVoxel : public VoxelElement{
    // defines that the voxel is a solid for other voxels. Not a liquid or gas
    State GetState() const override { return State::Solid; }; 

    // defines that the voxel will change the collider mesh of the voxel it is in for physics simulations when moved
    bool ShouldTriggerDirtyColliders() const override { return true; }; 

    // defines that the voxel will be part of that mesh. For eg. it wouldn't make sense to generate a solid mesh for liquids or gasses
    bool IsSolidCollider() const override { return true; };

	bool Step(ChunkMatrix* matrix) override;
}

bool Volume::MyVoxel::Step(ChunkMatrix* matrix){
    Vec2i posBelow = this->position + vector::DOWN;
    Volume::VoxelElement voxelBelow* = matrix->VirtualGetAt(posBelow);

    if(voxelBelow && below->GetState() != Volume::State::Solid){
        this->Swap(posBelow, *matrix);
        return true;
    }
    else{ // Voxel is a solid (or non-existent)
        this->DieAndReplace(*matrix, "Empty");
        return true;
    }

    return false;
}
```

### Solid voxel

> Volume::VoxelSolid

### Liquid voxel

> Volume::VoxelLiquid

### Gas voxel

> Volume::VoxelGas

### Empty voxel

> Volume::EmptyVoxel

Empty voxel is the only pre-registered voxel type avalible. Chunks can **not** have a null pointer inside their voxel 2D arrays, they expect to be always full with actual elements. This gives a purpose for the empty voxel element where you can use it instead of `nullptr` to prevent your aplication from crashing but also defining an empty space. The voxel does not move nor iteracts with anything

It is also spawned when a `Volume::VoxelGas` reaches a low enough pressure(amount) and replaces the gas

This "element" has zero density, heat conductivity and heat capacity. It is also fully transparent