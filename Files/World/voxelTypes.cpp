#include "voxelTypes.h"

#include <iostream>

Volume::FireVoxel::FireVoxel(Vec2i position, Temperature temp, float pressure) : VoxelGas("Fire", position, temp, pressure){ }

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

//Volume::IronVoxel::IronVoxel(Vec2i position, Temperature temp) : VoxelSolid("Iron", position, temp){ } 
//
//
//bool Volume::IronVoxel::Step(ChunkMatrix *matrix)
//{
//    if(rand() % 100 < 3){
//        DieAndReplace(*matrix, "Rust");
//        return true;
//    }
//    return VoxelSolid::Step(matrix);
//}
