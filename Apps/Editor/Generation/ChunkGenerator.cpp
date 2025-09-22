#include "ChunkGenerator.h"

Volume::Chunk *Generator::GenerateEmptyChunk(const Vec2i &pos, ChunkMatrix &matrix)
{
    Volume::Chunk* chunk = new Volume::Chunk(pos);

    for(int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y){
            chunk->voxels[y][x] = CreateVoxelElement(
                "Empty",
                Vec2i(
                    x + pos.x * Volume::Chunk::CHUNK_SIZE, 
                    y + pos.y * Volume::Chunk::CHUNK_SIZE
                ),
                20.0f,
                Volume::Temperature(21.0f),
                true
            );
        }
    }

    return chunk;
}