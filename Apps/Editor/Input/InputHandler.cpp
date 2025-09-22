#include "Input/InputHandler.h"

#include <algorithm>

#include <SDL_keycode.h>
#include <GameEngine.h>
#include <Registry/VoxelObjectRegistry.h>
#include "Editor.h"

Input::MouseData Input::mouseData = {};

void Input::OnMouseScroll(int yOffset)
{
    
}

void Input::OnKeyboardDown(int key)
{
    Vec2f worldMousePos = ChunkMatrix::MousePosToWorldPos(
        GameEngine::instance->GetMousePos(), 
        GameEngine::renderer->GetCameraOffset()
    );
}
void Input::OnKeyboardUp(int key)
{

}
void Input::OnMouseButtonDown(int button)
{
    GameEngine::instance->chunkMatrix.voxelMutex.lock();

    switch (button)
    {
    case SDL_BUTTON_LEFT:
        GameEngine::instance->chunkMatrix.PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            "Solid",
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            true,
            0,
            20
        );
        break;
    case SDL_BUTTON_RIGHT:
        GameEngine::instance->chunkMatrix.PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            "Empty",
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            true,
            0,
            20
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