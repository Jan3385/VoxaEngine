#include "Game.h"

#include "Rendering/ImGuiRenderer.h"
#include "Input/InputHandler.h"
#include "World/ChunkGenerator.h"
#include "World/RegisterVoxels.h"

#include <Registry/VoxelObjectRegistry.h>


Player *Editor::player = nullptr;

void Editor::OnInitialize()
{
    Registry::VoxelObjectProperty *playerProperties = Registry::VoxelObjectRegistry::GetProperties("Player");
    Editor::player = new Player(
        &GameEngine::instance->chunkMatrix,
        playerProperties->voxelData,
        playerProperties->densityOverride
    );
    GameEngine::instance->SetPlayer(Editor::player);
    GameEngine::physics->physicsObjects.push_back(Editor::player);
    
    GameEngine::instance->chunkMatrix.ChunkGeneratorFunction = ChunkGenerator::GenerateChunk;
}

void Editor::OnShutdown()
{
    delete Editor::player;
    Editor::player = nullptr;
}

void Editor::Update(float deltaTime)
{
    GameEngine::renderer->SetCameraPosition(Editor::player->GetPosition());
    Editor::player->UpdatePlayer(GameEngine::instance->chunkMatrix, deltaTime);
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

    int cursorSize = Input::mouseData.placementRadius * 2 + 1;
    cursorSize = Editor::player->gunEnabled ? 1 : cursorSize;

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

void Editor::RegisterVoxels()
{
    Registry::RegisterGameVoxels();
}

void Editor::RegisterVoxelObjects()
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
