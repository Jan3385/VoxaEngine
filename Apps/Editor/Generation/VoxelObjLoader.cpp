#include "VoxelObjLoader.h"
#include <Registry/VoxelObjectRegistry.h>

void ObjLoader::InsertVoxelsFromFileIntoScene(const std::string &filePath, EditorScene *scene)
{
    VoxelDataObj voxelData;
    bool loadingColor = Editor::instance.stateStorage.loadColorFromBMP;
    Registry::VoxelObjectRegistry::GetVoxelsFromFile(voxelData, filePath, loadingColor);

    AABB boundingBox = AABB(
        Vec2f(0, 0),
        Vec2f(static_cast<float>(voxelData[0].size()), static_cast<float>(voxelData.size()))
    );

    AABB sceneAABB = AABB(
        Vec2f(0, 0),
        Vec2f(
            static_cast<float>(scene->GetChunkSize().x * Volume::Chunk::CHUNK_SIZE),
            static_cast<float>(scene->GetChunkSize().y * Volume::Chunk::CHUNK_SIZE)
        )
    );

    if (!sceneAABB.Contains(boundingBox)) {
        std::cerr << "[ObjLoader] Voxel object from file " << filePath << " does not fit into the scene!" << std::endl;
        return;
    }

    Vec2f newObjCorner = sceneAABB.GetCenter() - (boundingBox.size / 2.0f) + Vec2i(Volume::Chunk::CHUNK_SIZE, Volume::Chunk::CHUNK_SIZE);
    boundingBox.corner = newObjCorner;

    for(size_t y = 0; y < voxelData.size(); y++) {
        for(size_t x = 0; x < voxelData[y].size(); x++) {
            const Registry::VoxelData &voxel = voxelData[y][x];
            
            if(voxel.color.a == 0) continue; // skip

            Vec2i voxelPos = Vec2i(x, y) + boundingBox.corner;

            Volume::VoxelElement *voxelElement = scene->chunkMatrix->PlaceVoxelAt(
                voxelPos,
                "Solid",
                Volume::Temperature(21.0f),
                true,
                1.0f,
                true
            );
            if(voxelElement)
                voxelElement->color = voxel.color;
        }
    }
}