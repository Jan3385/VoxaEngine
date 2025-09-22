#include "SolidSimpleVoxel.h"

Volume::SolidSimpleVoxel::SolidSimpleVoxel(std::string id, Vec2i position, Temperature temp, float amount)
    : VoxelElement(id, position, temp, amount)
{
}