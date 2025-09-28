#pragma once

#include "Editor.h"

namespace ObjLoader{
    using VoxelDataObj = std::vector<std::vector<Registry::VoxelData>>;
    void InsertVoxelsFromFileIntoScene(const std::string& filePath, EditorScene* scene);
}