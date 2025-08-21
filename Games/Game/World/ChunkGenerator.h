#pragma once

#include "World/Chunk.h"

namespace ChunkGenerator{
    Volume::Chunk* GenerateChunk(const Vec2i &chunkPos);
    Volume::Chunk* FillChunkWith(std::string materialID, bool unmovable, Volume::Chunk* chunk);
}