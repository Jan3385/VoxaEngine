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

Volume::Chunk *Generator::GenerateOxygenFilledChunk(const Vec2i &pos, ChunkMatrix &matrix)
{
    Volume::Chunk* chunk = new Volume::Chunk(pos);

    for(int x = 0; x < Volume::Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Volume::Chunk::CHUNK_SIZE; ++y){
            chunk->voxels[y][x] = CreateVoxelElement(
                "Oxygen",
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

void Generator::SetNewMatrix(const Vec2i &size, EditorScene::Type type)
{
    ChunkMatrix* newMatrix = new ChunkMatrix();

    switch (Editor::instance.stateStorage.selectedSceneType)
    {
    case EditorScene::Type::Sandbox:
        newMatrix->ChunkGeneratorFunction = Generator::GenerateOxygenFilledChunk;
        break;
    case EditorScene::Type::ObjectEditor:
    default:
        newMatrix->ChunkGeneratorFunction = Generator::GenerateEmptyChunk;
        break;
    }

    for(int x = 1; x <= size.x; ++x)
        for(int y = 1; y <= size.y; ++y)
            newMatrix->GenerateChunk(Vec2i(x, y));

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

    Editor::instance.scenes.push_back(
        EditorScene(
            editorTypeToStr[static_cast<int>(type)-1] + " " + std::to_string(Editor::instance.scenes.size() + 1),
            type,
            newMatrix,
            size
        )
    );
    
    Editor::instance.activeSceneIndex = Editor::instance.scenes.size() - 1;

    Editor::instance.GetActiveScene()->SetNewChunkSize(size);
}

void Generator::ExpandMatrixToSize(const Vec2i &size)
{
    EditorScene* activeScene = Editor::instance.GetActiveScene();

    if(!activeScene) return;

    Vec2i currentSize = activeScene->GetChunkSize();

    if(size.x <= currentSize.x &&
       size.y <= currentSize.y){
        std::cout << "Generator expand size smaller or same than current size, aborting\n";
        std::cout << "Current size: " << currentSize << ", requested size: " << size << std::endl;
        return;
    }

    for(int x = 1; x <= size.x; ++x)
        for(int y = 1; y <= size.y; ++y)
            GameEngine::instance->GetActiveChunkMatrix()->GenerateChunk(Vec2i(x, y));
    activeScene->SetNewChunkSize(size);
}
