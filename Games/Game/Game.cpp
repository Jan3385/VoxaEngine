#include "Game.h"

#include "Rendering/ImGuiRenderer.h"
#include "Input/InputHandler.h"

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

void Game::PhysicsUpdate(float deltaTime)
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
