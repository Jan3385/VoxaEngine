#include "Game.h"

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

    if(GameEngine::NoClip != Game::player->GetNoClip())
        Game::player->SetNoClip(GameEngine::NoClip);
}

void Game::FixedUpdate(float fixedDeltaTime)
{

}

void Game::PhysicsUpdate(float deltaTime)
{

}

void Game::Render()
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
