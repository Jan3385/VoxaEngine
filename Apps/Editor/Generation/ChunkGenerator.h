#pragma once

#include <GameEngine.h>

namespace Generator{
    Volume::Chunk *GenerateEmptyChunk(const Vec2i& pos, ChunkMatrix& matrix);
    void SetNewMatrix(const Vec2i& size);
    void ExpandMatrixToSize(const Vec2i& size);
}