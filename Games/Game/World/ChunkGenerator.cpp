#include "ChunkGenerator.h"
#include <Math/Random.h>
#include <Math/Noise.h>
#include <World/ChunkMatrix.h>

using namespace Volume;

Chunk* ChunkGenerator::GenerateChunk(const Vec2i &chunkPos, ChunkMatrix &chunkMatrix){
    constexpr int seed = 123;

    Random gen(chunkPos.x * 1234 + chunkPos.y * 5678 + seed);
    Chunk* chunk = new Chunk(chunkPos);

    // some basic debug stuff
    if (chunkPos.y == 4 && chunkPos.x <= 3)
        if(chunkPos.x == 2)
            return FillChunkWith("Water", false, chunk);
        else
            return FillChunkWith("Sand", false, chunk);
    else if (chunkPos.y < 5)
        return FillChunkWith("Oxygen", false, chunk);

    ChunkBoolArray chunkData = GenerateArrayFromNoise(chunkPos, seed, chunkMatrix);

    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            if(chunkData[y][x]){
                chunk->voxels[y][x] = CreateVoxelElement(
                    GetMaterialAtYLevel(y + chunkPos.y * Chunk::CHUNK_SIZE, gen),
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE),
                    20,
                    Temperature(21),
                    true
                );
            }else{
                chunk->voxels[y][x] = CreateVoxelElement(
                    "Oxygen",
                    Vec2i(x + chunkPos.x * Chunk::CHUNK_SIZE, y + chunkPos.y * Chunk::CHUNK_SIZE),
                    1,
                    Temperature(21),
                    true
                );
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

std::string ChunkGenerator::GetMaterialAtYLevel(int yLevel, Random &gen)
{
    if(yLevel < 380) return "Grass";
    if(yLevel < 400) if(gen.GetInt(380, yLevel) < 390) return "Grass";
    if(yLevel < 700) return "Dirt";
    if(yLevel < 740) if(gen.GetInt(700, yLevel) < 720) return "Dirt";

    return "Stone";
}

ChunkGenerator::ChunkBoolArray ChunkGenerator::GenerateArrayFromNoise(Vec2i chunkPos, int seed, ChunkMatrix &chunkMatrix)
{
    ChunkBoolArray chunkData;

    for (int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x) {
        for (int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y) {
            Vec2f noisePos = Vec2f(x + chunkPos.x * Volume::Chunk::CHUNK_SIZE, y + chunkPos.y * Volume::Chunk::CHUNK_SIZE);
            noisePos = noisePos / GEN_SCALE;

            float noiseValue = Noise::ValueNoise(noisePos, seed);

            chunkData[y][x] = noiseValue > GEN_AIR_PERCENTAGE/100.0f; 
        }
    }

    return chunkData;
}