#include "Editor.h"

#include <Registry/VoxelObjectRegistry.h>

#include "Generation/ChunkGenerator.h"
#include "Generation/VoxelRegistryGenerator.h"
#include "Input/InputHandler.h"

void Editor::OnInitialize()
{
    Registry::VoxelObjectProperty *playerProperties = Registry::VoxelObjectRegistry::GetProperties("Player");
    
    GameEngine::instance->chunkMatrix.ChunkGeneratorFunction = Generator::GenerateEmptyChunk;
}

void Editor::OnShutdown()
{

}

void Editor::Update(float deltaTime)
{
    Vec2f movementVector = Vec2f(0, 0);
    if(GameEngine::MovementKeysHeld[0]) //W
        movementVector += vector::UP;
    if(GameEngine::MovementKeysHeld[1]) //S
        movementVector += vector::DOWN;
    if(GameEngine::MovementKeysHeld[2]) //A
        movementVector += vector::LEFT;
    if(GameEngine::MovementKeysHeld[3]) //D
        movementVector += vector::RIGHT;

    this->cameraPosition += movementVector * deltaTime * 50.0f;
    GameEngine::renderer->SetCameraPosition(this->cameraPosition);

    if(Input::mouseData.leftButtonDown)
    {
        GameEngine::instance->chunkMatrix.PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            "Solid",
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            false,
            0,
            20
        );
    }
    if(Input::mouseData.rightButtonDown)
    {
        GameEngine::instance->chunkMatrix.PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            "Empty",
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            false,
            0,
            20
        );
    }
}

void Editor::FixedUpdate(float fixedDeltaTime)
{

}

void Editor::VoxelUpdate(float deltaTime)
{

}

void Editor::Render(glm::mat4 voxelProjection, glm::mat4 viewProjection)
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
    
    this->imguiRenderer.RenderPanel();

    // Set mouse based on if hovering over an element
    bool mouseOverUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if(mouseOverUI)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    else
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void Editor::RegisterVoxels()
{
    Registry::RegisterEditorVoxels();
}

void Editor::RegisterVoxelObjects()
{
    using namespace Registry;

}

void Editor::OnMouseScroll(int yOffset)
{
    Input::OnMouseScroll(yOffset);
}

void Editor::OnMouseButtonDown(int button)
{
    Input::OnMouseButtonDown(button);
}

void Editor::OnMouseButtonUp(int button)
{
    Input::OnMouseButtonUp(button);
}

void Editor::OnMouseMove(int x, int y)
{
    Input::OnMouseMove(x, y);
}

void Editor::OnKeyboardDown(int key)
{
    Input::OnKeyboardDown(key);
}

void Editor::OnKeyboardUp(int key)
{
    Input::OnKeyboardUp(key);
}

void Editor::OnWindowResize(int newX, int newY)
{

}