#include "ChunkGenerator.h"

using namespace Volume;

Chunk* ChunkGenerator::GenerateChunk(const Vec2i &pos){
    Chunk* chunk = new Chunk(pos);

    for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
            //Create voxel determining on its position
            if (pos.y > 4) {
                if(pos.y == 5 && y <= 5){
                    chunk->voxels[y][x] = new VoxelSolid
                        ("Grass", 
                        Vec2i(x + pos.x * Chunk::CHUNK_SIZE,y + pos.y * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        true, 20);
                }else{
                    chunk->voxels[y][x] = new VoxelSolid
                        ("Dirt", 
                        Vec2i(x + pos.x * Chunk::CHUNK_SIZE,y + pos.y * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        true, 20);
                }
            }
            else if (pos.y == 4 && pos.x <= 3) {
                if(pos.x == 2)
                    chunk->voxels[y][x] = new VoxelLiquid
                        ("Water", 
                        Vec2i(x + pos.x * Chunk::CHUNK_SIZE, y + pos.y * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        20);
                else
                    chunk->voxels[y][x] = new VoxelSolid
                        ("Sand", 
                        Vec2i(x + pos.x * Chunk::CHUNK_SIZE, y + pos.y * Chunk::CHUNK_SIZE), 
                        Temperature(21), 
                        false, 20);
            }
            else {
                chunk->voxels[y][x] = new VoxelGas
                    ("Oxygen", 
                    Vec2i(x + pos.x * Chunk::CHUNK_SIZE, y + pos.y * Chunk::CHUNK_SIZE),
                    Temperature(21), 1);
            }
        }
    }

    return chunk;
}
