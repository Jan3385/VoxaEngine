#pragma once

#include "World/Chunk.h"
#include "Math/Random.h"

namespace ChunkGenerator{
    constexpr int GEN_AIR_PERCENTAGE = 45;
    constexpr float GEN_SCALE = 50.0f;
    constexpr float GEN_X_MULTIPLIER = 1.9f; // makes the X axis n times wider

    Volume::Chunk* GenerateChunk(const Vec2i &chunkPos, ChunkMatrix &chunkMatrix);
    Volume::Chunk* FillChunkWith(std::string materialID, bool unmovable, Volume::Chunk* chunk);

    std::string GetMaterialAtYLevel(int yLevel, Random &gen);

    using ChunkBoolArray = std::array<std::array<bool, Volume::Chunk::CHUNK_SIZE>, Volume::Chunk::CHUNK_SIZE>;
    ChunkBoolArray GenerateArrayFromNoise(Vec2i chunkPos, int seed, ChunkMatrix &chunkMatrix);
}