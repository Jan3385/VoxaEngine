#include "Input/InputHandler.h"

#include <algorithm>

#include <SDL_keycode.h>
#include <GameEngine.h>
#include <Registry/GameObjectRegistry.h>
#include "Rendering/ImGuiRenderer.h"
#include "Game.h"

Input::MouseData Input::mouseData = {};

void Input::OnMouseScroll(int yOffset)
{
    mouseData.placementRadius += yOffset;
    mouseData.placementRadius = std::clamp(mouseData.placementRadius, 0, 10);
}

void Input::OnKeyboardDown(int key)
{
    Vec2f worldMousePos = ChunkMatrix::MousePosToWorldPos(
        GameEngine::instance->mousePos, 
        GameEngine::renderer->GetCameraOffset()
    );

    switch(key){
        case SDLK_t:
            std::cout << "Creating barrel" << std::endl;
            Registry::CreateGameObject("Barrel", worldMousePos, &GameEngine::instance->chunkMatrix, GameEngine::physics);
            break;
        case SDLK_z:
            Registry::CreateGameObject("Ball", worldMousePos, &GameEngine::instance->chunkMatrix, GameEngine::physics);
            break;
        case SDLK_u:
            Registry::CreateGameObject("Crate", worldMousePos, &GameEngine::instance->chunkMatrix, GameEngine::physics);
            break;
    }
}
void Input::OnKeyboardUp(int key)
{
    switch (key)
    {
    case SDLK_F1:
        ImGuiRenderer::fullImGui = !ImGuiRenderer::fullImGui;
        break;
    }   
}
void Input::OnMouseButtonDown(int button)
{
    GameEngine::instance->chunkMatrix.voxelMutex.lock();

    switch (button)
    {
    case SDL_BUTTON_LEFT:
        if(Game::player->gunEnabled){
            Game::player->FireGun(
                GameEngine::instance->chunkMatrix
            );
        }else{
            GameEngine::instance->chunkMatrix.PlaceVoxelsAtMousePosition(
                GameEngine::instance->mousePos,
                mouseData.placeVoxelType,
                GameEngine::renderer->GetCameraOffset(),
                Volume::Temperature(mouseData.placeVoxelTemperature),
                mouseData.placeUnmovableSolidVoxels,
                mouseData.placementRadius
            );
        } 
        break;
    case SDL_BUTTON_RIGHT:
        GameEngine::instance->chunkMatrix.ExplodeAtMousePosition(
            GameEngine::instance->mousePos,
            15,
            GameEngine::renderer->GetCameraOffset()
        );
        break;
    }

    GameEngine::instance->chunkMatrix.voxelMutex.unlock();
}
void Input::OnMouseButtonUp(int button)
{

}
void Input::OnMouseMove(int x, int y)
{
    
}