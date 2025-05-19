#include "../voxelTypes.h"
#include "../Chunk.h"

using namespace Volume;

EmptyVoxel::EmptyVoxel(Vec2i position)
    : VoxelElement("Empty", position, Temperature(0), 0) {  };

bool EmptyVoxel::Step(ChunkMatrix* matrix){
    // Search for nearby voxels to fill the empty space
    for(Vec2i dir : vector::AROUND4){
        VoxelElement* next = matrix->VirtualGetAt_NoLoad(this->position + dir);
        if(next && next->GetState() == State::Gas && next->id != "Empty"){
            // Fill the empty space with the found voxel
            int amount = next->amount;
            matrix->PlaceVoxelAt(this->position, next->id, next->temperature, false, amount/10.0f, true);
            next->amount -= amount/10.0f;
            return true;
        }
    }
    return false;
}