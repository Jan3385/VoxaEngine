#include "Input/InputHandler.h"

#include <algorithm>

#include <SDL_keycode.h>
#include <GameEngine.h>
#include <Registry/VoxelObjectRegistry.h>
#include "Editor.h"

Input::MouseData Input::mouseData = {};

void Input::OnMouseScroll(int yOffset)
{
    Input::mouseData.brushRadius = std::clamp(
        Input::mouseData.brushRadius + yOffset,
        0,
        10
    );   
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
    GameEngine::instance->GetActiveChunkMatrix()->voxelMutex.lock();

    if(button == SDL_BUTTON_LEFT)
        mouseData.leftButtonDown = true;
    if(button == SDL_BUTTON_RIGHT)
        mouseData.rightButtonDown = true;

    switch (button)
    {
    case SDL_BUTTON_LEFT:
        mouseData.leftButtonDown = true;
        break;
    case SDL_BUTTON_RIGHT:
        mouseData.rightButtonDown = true;
        break;
    }

    GameEngine::instance->GetActiveChunkMatrix()->voxelMutex.unlock();
}
void Input::OnMouseButtonUp(int button)
{
    if(button == SDL_BUTTON_LEFT)
        mouseData.leftButtonDown = false;
    if(button == SDL_BUTTON_RIGHT)
        mouseData.rightButtonDown = false;
}
void Input::OnMouseMove(int x, int y)
{
    
}