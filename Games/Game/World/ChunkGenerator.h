#pragma once

#include "World/Chunk.h"
#include "Math/Random.h"

namespace ChunkGenerator{
    constexpr int GEN_FILL_PERCENTAGE = 60;

    Volume::Chunk* GenerateChunk(const Vec2i &chunkPos, ChunkMatrix &chunkMatrix);
    Volume::Chunk* FillChunkWith(std::string materialID, bool unmovable, Volume::Chunk* chunk);
    using ChunkBoolArray = std::array<std::array<bool, Volume::Chunk::CHUNK_SIZE>, Volume::Chunk::CHUNK_SIZE>;
    ChunkBoolArray FillChunkDataRandom(Random& gen, ChunkMatrix &chunkMatrix, const Vec2i &chunkPos);

    ChunkBoolArray SmoothOutChunkData(const ChunkBoolArray &chunkData, Vec2i chunkPos, ChunkMatrix &chunkMatrix);
    int GetSurroundingSolidCount(const ChunkBoolArray &chunkData, int x, int y, Vec2i chunkPos, ChunkMatrix &chunkMatrix);
}