#include "ChunkGenerator.h"

#include "Editor.h"

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

void Generator::SetNewMatrix(const Vec2i &size)
{
    ChunkMatrix* newMatrix = new ChunkMatrix();
    newMatrix->ChunkGeneratorFunction = Generator::GenerateEmptyChunk;

    for(int x = 1; x <= size.x; ++x)
        for(int y = 1; y <= size.y; ++y)
            newMatrix->GenerateChunk(Vec2i(x, y));
    Editor::instance.stateStorage.SetNewChunkSize(size);

    // Move camera to the center of the new matrix chunks
    Vec2f center = Vec2f(
        (size.x * Volume::Chunk::CHUNK_SIZE) / 2.0f + Volume::Chunk::CHUNK_SIZE,
        (size.y * Volume::Chunk::CHUNK_SIZE) / 2.0f + Volume::Chunk::CHUNK_SIZE
    );
    center = center - Vec2i(
        (Editor::instance.imguiRenderer.panelSideWidth / Volume::Chunk::RENDER_VOXEL_SIZE) / 2, 
        -(Editor::instance.imguiRenderer.panelBottomHeight / Volume::Chunk::RENDER_VOXEL_SIZE) / 2
    );
    Editor::instance.cameraPosition = center;
    GameEngine::instance->SetActiveChunkMatrix(newMatrix);
}

void Generator::ExpandMatrixToSize(const Vec2i &size)
{
    if(size.x <= Editor::instance.stateStorage.GetChunkSize().x &&
       size.y <= Editor::instance.stateStorage.GetChunkSize().y){
        std::cout << "Generator expand size smaller or same than current size, aborting\n";
        std::cout << "Current size: " << Editor::instance.stateStorage.GetChunkSize() << ", requested size: " << size << std::endl;
        return;
    }

    for(int x = 1; x <= size.x; ++x)
        for(int y = 1; y <= size.y; ++y)
            GameEngine::instance->GetActiveChunkMatrix()->GenerateChunk(Vec2i(x, y));
    Editor::instance.stateStorage.SetNewChunkSize(size);
}
