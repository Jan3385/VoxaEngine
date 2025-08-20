#include "Game.h"

#include "Rendering/ImGuiRenderer.h"
#include "Input/InputHandler.h"
#include "World/ChunkGenerator.h"

#include <Registry/GameObjectRegistry.h>


Player *Game::player = nullptr;

void Game::OnInitialize()
{
    Registry::GameObjectProperty *playerProperties = Registry::GameObjectRegistry::GetProperties("Player");
    Game::player = new Player(
        &GameEngine::instance->chunkMatrix,
        playerProperties->voxelData,
        playerProperties->densityOverride
    );
    GameEngine::instance->SetPlayer(Game::player);
    GameEngine::physics->physicsObjects.push_back(Game::player);
    
    GameEngine::instance->chunkMatrix.ChunkGeneratorFunction = ChunkGenerator::GenerateChunk;
}

void Game::OnShutdown()
{

}

void Game::Update(float deltaTime)
{
    GameEngine::renderer->SetCameraPosition(Game::player->GetPosition());
    Game::player->UpdatePlayer(GameEngine::instance->chunkMatrix, deltaTime);
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
        GameEngine::instance->mousePos, 
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
    using namespace Registry;
}

void Game::RegisterVoxelObjects()
{
    using namespace Registry;
    
    GameObjectRegistry::RegisterGameObject(
        "Player",
        GameObjectBuilder(GameObjectType::PhysicsObject)
            .SetDensityOverride(985.0f)
            .SetVoxelFileName("Player")
            .Build()
    );
    GameObjectRegistry::RegisterGameObject(
        "Barrel",
        GameObjectBuilder(GameObjectType::PhysicsObject)
            .SetDensityOverride(400.0f)
            .SetVoxelFileName("Barrel")
            .Build()
    );
    GameObjectRegistry::RegisterGameObject(
        "Ball",
        GameObjectBuilder(GameObjectType::PhysicsObject)
            .SetVoxelFileName("Ball")
            .Build()
    );
    GameObjectRegistry::RegisterGameObject(
        "Crate",
        GameObjectBuilder(GameObjectType::GameObject)
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
