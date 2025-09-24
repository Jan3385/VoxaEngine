#include "Editor.h"

#include <Registry/VoxelObjectRegistry.h>

#include "Generation/ChunkGenerator.h"
#include "Generation/VoxelRegistryGenerator.h"
#include "Input/InputHandler.h"

Editor Editor::instance = Editor();

void Editor::OnInitialize()
{
    GameEngine::instance->GetActiveChunkMatrix()->ChunkGeneratorFunction = Generator::GenerateEmptyChunk;
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
        std::vector<Volume::VoxelElement*> placedVoxels;
        placedVoxels = GameEngine::instance->GetActiveChunkMatrix()->PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            "Solid",
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            false,
            Input::mouseData.brushRadius,
            20
        );
        for(auto& placedVoxel : placedVoxels){
            if(placedVoxel == nullptr) continue;
            placedVoxel->color = Input::mouseData.placeColor;
        }
    }
    if(Input::mouseData.rightButtonDown)
    {
        GameEngine::instance->GetActiveChunkMatrix()->PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            "Empty",
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            false,
            Input::mouseData.brushRadius,
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
    if(this->stateStorage.GetChunkSize() != Vec2i(0, 0)){
        glm::vec4 outlineColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        Vec2i chunkSizeEnd = this->stateStorage.GetChunkSize() * Volume::Chunk::CHUNK_SIZE + Vec2i(Volume::Chunk::CHUNK_SIZE, Volume::Chunk::CHUNK_SIZE);
        std::vector<glm::vec2> points = {
            {Volume::Chunk::CHUNK_SIZE, Volume::Chunk::CHUNK_SIZE},
            {chunkSizeEnd.x, Volume::Chunk::CHUNK_SIZE},
            {chunkSizeEnd.x, chunkSizeEnd.y},
            {Volume::Chunk::CHUNK_SIZE, chunkSizeEnd.y}
        };
        GameEngine::renderer->DrawClosedShape(points, outlineColor, voxelProjection, 1.0f);
    }

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
        Input::mouseData.brushRadius * 2 + 1
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

void Editor::OnSceneChange(ChunkMatrix* oldMatrix, ChunkMatrix* newMatrix)
{
    delete oldMatrix;
}