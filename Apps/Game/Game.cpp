#include "Game.h"

#include "Rendering/ImGuiRenderer.h"
#include "Input/InputHandler.h"
#include "World/ChunkGenerator.h"
#include "World/RegisterVoxels.h"

#include <Registry/VoxelObjectRegistry.h>


Player *Game::player = nullptr;

void Game::OnInitialize()
{
    Registry::VoxelObjectProperty *playerProperties = Registry::VoxelObjectRegistry::GetProperties("Player");
    Game::player = new Player(
        GameEngine::instance->GetActiveChunkMatrix(),
        playerProperties->voxelData,
        playerProperties->densityOverride
    );
    GameEngine::instance->SetPlayer(Game::player);
    GameEngine::instance->GetActiveChunkMatrix()->physicsObjects.push_back(Game::player);

    GameEngine::instance->GetActiveChunkMatrix()->ChunkGeneratorFunction = ChunkGenerator::GenerateChunk;
}

void Game::OnShutdown()
{
    delete Game::player;
    Game::player = nullptr;
}

void Game::Update(float deltaTime)
{
    GameEngine::renderer->SetCameraPosition(Game::player->GetPosition());
    Game::player->UpdatePlayer(*GameEngine::instance->GetActiveChunkMatrix(), deltaTime);
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

    int cursorSize = Input::mouseData.placementRadius * 2 + 1;
    cursorSize = Game::player->gunEnabled ? 1 : cursorSize;

    GameEngine::renderer->RenderCursor(
        mousePosInWorldInt,
        voxelProjection,
        cursorSize
    );

    ImGuiRenderer::RenderDebugPanel();

    // Set mouse based on if hovering over an element
    bool mouseOverUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if(mouseOverUI)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void Game::RegisterVoxels()
{
    Registry::RegisterGameVoxels();
}

void Game::RegisterVoxelObjects()
{
    using namespace Registry;
    
    VoxelObjectRegistry::RegisterVoxelObject(
        "Player",
        VoxelObjectBuilder(VoxelObjectType::PhysicsObject)
            .SetDensityOverride(985.0f)
            .SetVoxelFileName("Player")
            .Build()
    );
    VoxelObjectRegistry::RegisterVoxelObject(
        "Barrel",
        VoxelObjectBuilder(VoxelObjectType::PhysicsObject)
            .SetDensityOverride(400.0f)
            .SetVoxelFileName("Barrel")
            .Build()
    );
    VoxelObjectRegistry::RegisterVoxelObject(
        "Ball",
        VoxelObjectBuilder(VoxelObjectType::PhysicsObject)
            .SetVoxelFileName("Ball")
            .Build()
    );
    VoxelObjectRegistry::RegisterVoxelObject(
        "Crate",
        VoxelObjectBuilder(VoxelObjectType::VoxelObject)
            .SetVoxelFileName("Crate")
            .Build()
    );
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
