#pragma once

#include <array>
#include <string>

#include <GameEngine.h>

#include "Editor.h"

namespace Generator{
    Volume::Chunk *GenerateEmptyChunk(const Vec2i& pos, ChunkMatrix& matrix);
    Volume::Chunk *GenerateOxygenFilledChunk(const Vec2i& pos, ChunkMatrix& matrix);
    void SetNewMatrix(const Vec2i& size, EditorScene::Type type);
    void ExpandMatrixToSize(const Vec2i& size);

    static std::array<std::string, 2> editorTypeToStr = {
        "ObjEditor",
        "Sandbox"
    };
}