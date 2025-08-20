#include "ChunkGenerator.h"

using namespace Volume;

Chunk* ChunkGenerator::GenerateChunk(const Vec2i &chunkPos){
    Chunk* chunk = new Chunk(chunkPos);

    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            //Create voxel determining on its position
            if (chunkPos.y > 4) {
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
            else if (chunkPos.y == 4 && chunkPos.x <= 3) {
                if(chunkPos.x == 2)
                    chunk->voxels[y][x] = new VoxelLiquid
                        ("Water", 
                        Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        20);
                else
                    chunk->voxels[y][x] = new VoxelSolid
                        ("Sand", 
                        Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        false, 20);
            }
            else {
                chunk->voxels[y][x] = new VoxelGas
                    ("Oxygen", 
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE),
                    Temperature(21), 1);
            }
        }
    }

    return chunk;
}
