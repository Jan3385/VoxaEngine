#include "Empty.h"
#include "../Chunk.h"

#include <iostream>

using namespace Volume;

EmptyVoxel::EmptyVoxel(Vec2i position)
    : VoxelElement("Empty", position, Temperature(0), 0) {  };

bool EmptyVoxel::Step(ChunkMatrix* matrix){
    // Search for nearby voxels to fill the empty space
    for(Vec2i dir : vector::AROUND8){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + dir);
        if(next && next->GetState() == State::Gas && next->properties != this->properties){
            // Fill the empty space with the found voxel
            matrix->PlaceVoxelAt(this->position, next->id, next->temperature, false, next->amount/2.0f, true);
            next->amount -= next->amount/2.0f;
            return true;
        }
    }
    return false;
}