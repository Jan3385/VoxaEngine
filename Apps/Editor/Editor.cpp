#include "Editor.h"

#include <Registry/VoxelObjectRegistry.h>

#include "Generation/ChunkGenerator.h"
#include "Generation/VoxelRegistryGenerator.h"
#include "Input/InputHandler.h"

void Game::OnInitialize()
{
    Registry::VoxelObjectProperty *playerProperties = Registry::VoxelObjectRegistry::GetProperties("Player");
    
    GameEngine::instance->chunkMatrix.ChunkGeneratorFunction = Generator::GenerateEmptyChunk;
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
    Registry::RegisterEditorVoxels();
}

void Game::RegisterVoxelObjects()
{
    using namespace Registry;

}

void Game::OnMouseScroll(int yOffset)
{
    Input::OnMouseScroll(yOffset);
}

void Game::OnMouseButtonDown(int button)
{
    Input::OnMouseButtonDown(button);
}

void Game::OnMouseButtonUp(int button)
{
    Input::OnMouseButtonUp(button);
}

void Game::OnMouseMove(int x, int y)
{
    Input::OnMouseMove(x, y);
}

void Game::OnKeyboardDown(int key)
{
    Input::OnKeyboardDown(key);
}

void Game::OnKeyboardUp(int key)
{
    Input::OnKeyboardUp(key);
}

void Game::OnWindowResize(int newX, int newY)
{

}