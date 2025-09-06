#include "World/Voxels/Empty.h"
#include "World/ChunkMatrix.h"

#include <iostream>

using namespace Volume;

EmptyVoxel::EmptyVoxel(Vec2i position)
    : VoxelElement("Empty", position, Temperature(0), 0) {  };

bool EmptyVoxel::Step(ChunkMatrix* matrix){
    //return false;

    
    // Search for nearby voxels to fill the empty space
    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + dir);
        if(next && next->GetState() == State::Gas && next->properties != this->properties && next->amount > VoxelGas::MinimumGasAmount){
            // Fill the empty space with the found voxel)
            float movedAmount = next->amount / 4.0f;
            matrix->PlaceVoxelAt(this->position, next->id, next->temperature, false, movedAmount, true);
            next->amount -= movedAmount;
            return true;
        }
    }
    return false;
    
}