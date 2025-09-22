#include "Input/InputHandler.h"

#include <algorithm>

#include <SDL_keycode.h>
#include <GameEngine.h>
#include <Registry/VoxelObjectRegistry.h>
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
        GameEngine::instance->GetMousePos(), 
        GameEngine::renderer->GetCameraOffset()
    );

    switch(key){
        case SDLK_t:
            Registry::CreateVoxelObject("Barrel", worldMousePos, &GameEngine::instance->chunkMatrix, GameEngine::physics);
            break;
        case SDLK_z:
            Registry::CreateVoxelObject("Ball", worldMousePos, &GameEngine::instance->chunkMatrix, GameEngine::physics);
            break;
        case SDLK_u:
            Registry::CreateVoxelObject("Crate", worldMousePos, &GameEngine::instance->chunkMatrix, GameEngine::physics);
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
        if(Editor::player->gunEnabled){
            Editor::player->FireGun(
                GameEngine::instance->chunkMatrix
            );
        }else{
            GameEngine::instance->chunkMatrix.PlaceVoxelsAtMousePosition(
                GameEngine::instance->GetMousePos(),
                mouseData.placeVoxelType,
                GameEngine::renderer->GetCameraOffset(),
                Volume::Temperature(mouseData.placeVoxelTemperature),
                mouseData.placeUnmovableSolidVoxels,
                mouseData.placementRadius,
                mouseData.placeVoxelAmount
            );
        } 
        break;
    case SDL_BUTTON_RIGHT:
        GameEngine::instance->chunkMatrix.ExplodeAtMousePosition(
            GameEngine::instance->GetMousePos(),
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