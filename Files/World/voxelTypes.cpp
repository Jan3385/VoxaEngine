#include "voxelTypes.h"

#include <iostream>

Volume::FireVoxel::FireVoxel()
    : VoxelGas(VoxelType::Fire, Vec2i(0, 0))
{
    temperature.SetCelsius(40000);
}

Volume::FireVoxel::FireVoxel(Vec2i position)
    : VoxelGas(VoxelType::Fire, position)
{
    temperature.SetCelsius(40000);
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
    	this->DieAndReplace(*matrix, std::make_shared<VoxelGas>(VoxelType::Oxygen, this->position));
        return true;
    }
    VoxelGas::Step(matrix);
    return true;
}
