#include "voxelTypes.h"

#include <iostream>

Volume::FireVoxel::FireVoxel()
    : VoxelGas("Fire", Vec2i(0, 0), Temperature(40000))
{
}

Volume::FireVoxel::FireVoxel(Vec2i position, Temperature temp)
    : VoxelGas("Fire", position, temp)
{
}

Volume::FireVoxel::~FireVoxel()
{
}

bool Volume::FireVoxel::Step(ChunkMatrix *matrix)
{
    forcedLifetimeTime--;
    //15% chance to dissapear
    if (rand() % 100 < 15 || forcedLifetimeTime <= 0)
    {
    	this->DieAndReplace(*matrix, "Carbon_Dioxide");
        return true;
    }
    VoxelGas::Step(matrix);
    return true;
}
