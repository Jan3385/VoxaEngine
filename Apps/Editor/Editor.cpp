#include "Editor.h"

#include <Registry/VoxelObjectRegistry.h>


void Game::OnInitialize()
{
    Registry::VoxelObjectProperty *playerProperties = Registry::VoxelObjectRegistry::GetProperties("Player");
    
    GameEngine::instance->chunkMatrix.ChunkGeneratorFunction = GenerateEmptyChunk;
}

void Game::OnShutdown()
{

}

void Game::Update(float deltaTime)
{
    GameEngine::renderer->SetCameraPosition(Vec2i(100, 100));
}

void Game::FixedUpdate(float fixedDeltaTime)
{

}

void Game::VoxelUpdate(float deltaTime)
{

}

void Game::Render(glm::mat4 voxelProjection, glm::mat4 viewProjection)
{
    Vec2f mousePosInWorldF = ChunkMatrix::MousePosToWorldPos(
        GameEngine::instance->GetMousePos(), 
        GameEngine::renderer->GetCameraOffset()
    );

    glm::vec2 mousePosInWorldInt = glm::vec2(
        static_cast<int>(mousePosInWorldF.x), 
        static_cast<int>(mousePosInWorldF.y)
    );

    GameEngine::renderer->RenderCursor(
        mousePosInWorldInt,
        voxelProjection,
        1
    );

    // Set mouse based on if hovering over an element
    bool mouseOverUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if(mouseOverUI)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void Game::RegisterVoxels()
{
    using namespace Registry;
}

void Game::RegisterVoxelObjects()
{
    using namespace Registry;

}

void Game::OnMouseScroll(int yOffset)
{
    
}

void Game::OnMouseButtonDown(int button)
{
    
}

void Game::OnMouseButtonUp(int button)
{
    
}

void Game::OnMouseMove(int x, int y)
{
    
}

void Game::OnKeyboardDown(int key)
{
    
}

void Game::OnKeyboardUp(int key)
{
    
}

void Game::OnWindowResize(int newX, int newY)
{

}

Volume::Chunk *GenerateEmptyChunk(const Vec2i &pos, ChunkMatrix &matrix)
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
