#include "ChunkGenerator.h"
#include "Math/Random.h"

using namespace Volume;

Chunk* ChunkGenerator::GenerateChunk(const Vec2i &chunkPos){
    Random gen(chunkPos.x * 1234 + chunkPos.y * 5678);
    Chunk* chunk = new Chunk(chunkPos);

    // some basic debug stuff
    if (chunkPos.y == 4 && chunkPos.x <= 3)
        if(chunkPos.x == 2)
            return FillChunkWith("Water", false, chunk);
        else
            return FillChunkWith("Sand", false, chunk);
    else if (chunkPos.y < 5)
        return FillChunkWith("Oxygen", false, chunk);

    

    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            //Create voxel determining on its position
            if(chunkPos.y == 5 && y <= 5){
                chunk->voxels[y][x] = new VoxelSolid
                    ("Grass", 
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE,y + chunkPos.y * Chunk::CHUNK_SIZE), 
                    Temperature(21), 
                    true, 20);
            }else{
                chunk->voxels[y][x] = new VoxelSolid
                    ("Dirt", 
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE,y + chunkPos.y * Chunk::CHUNK_SIZE), 
                    Temperature(21), 
                    true, 20);
            }
        }
    }

    return chunk;
}

Volume::Chunk *ChunkGenerator::FillChunkWith(std::string materialID, bool unmovable, Volume::Chunk *chunk)
{
    Volume::VoxelProperty *prop = Registry::VoxelRegistry::GetProperties(materialID);
    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            Volume::VoxelElement* voxel = CreateVoxelElement(
                prop,
                materialID,
                Vec2i(x + chunk->GetPos().x * Chunk::CHUNK_SIZE, y + chunk->GetPos().y * Chunk::CHUNK_SIZE),
                20,
                Temperature(21),
                unmovable
            );
            chunk->voxels[y][x] = voxel;
        }
    }
    return chunk;
}
