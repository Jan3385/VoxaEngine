#include "Editor.h"

#include <Registry/VoxelObjectRegistry.h>

#include "Generation/ChunkGenerator.h"
#include "Generation/VoxelRegistryGenerator.h"
#include "Input/InputHandler.h"

#include "Generation/VoxelObjLoader.h"

Editor Editor::instance = Editor();

EditorScene *Editor::GetActiveScene()
{
    if (activeSceneIndex >= scenes.size()) {
        return nullptr;
    }

    return &scenes[activeSceneIndex];
}

void Editor::SwitchToScene(size_t index)
{
    if(index >= this->scenes.size()) {
        Debug::LogError("[Editor] Scene index out of bounds: " + std::to_string(index));
        return;
    }
    if(this->scenes[index].chunkMatrix == nullptr) {
        Debug::LogError("[Editor] Scene at index " + std::to_string(index) + " has no ChunkMatrix!");
        return;
    }

    ChunkMatrix* newMatrix = this->scenes[index].chunkMatrix;
    GameEngine::instance->SetActiveChunkMatrix(newMatrix);
}

void Editor::SetSimulationActive(bool state)
{
    if(state){
        if(Editor::instance.stateStorage.simulationFPS > 0.0f &&
          !Editor::instance.stateStorage.runSimulationAuto){

            Editor::instance.stateStorage.runSimulationAuto = true;
            GameEngine::instance->SetPauseVoxelSimulation(false);
        }
    }else{
        if(Editor::instance.stateStorage.runSimulationAuto){
            Editor::instance.stateStorage.runSimulationAuto = false;
            GameEngine::instance->SetPauseVoxelSimulation(true);
        }
    }
}

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
        std::string voxelID = "Solid";
        bool placeUnmovable = true;

        if(Editor::instance.stateStorage.selectedSceneType == EditorScene::Type::Sandbox)
        {
            placeUnmovable = Editor::instance.stateStorage.placedSimVoxelsAreUnmovable;

            switch (Editor::instance.stateStorage.selectedVoxelState)
            {
            case Volume::State::Gas:
                voxelID = "Oxygen";
                break;
            case Volume::State::Liquid:
                voxelID = "Water";
                break;
            case Volume::State::Solid:
                voxelID = "Dirt";
                break;
            }
        }

        std::vector<Volume::VoxelElement*> placedVoxels;
        placedVoxels = GameEngine::instance->GetActiveChunkMatrix()->PlaceVoxelsAtMousePosition(
            GameEngine::instance->GetMousePos(),
            voxelID,
            GameEngine::renderer->GetCameraOffset(),
            Volume::Temperature(21.0f),
            placeUnmovable,
            Input::mouseData.brushRadius,
            20
        );

        if(Editor::instance.stateStorage.selectedSceneType == EditorScene::Type::ObjectEditor){
            for(auto& placedVoxel : placedVoxels){
                if(placedVoxel == nullptr) continue;
                placedVoxel->color = Input::mouseData.placeColor;
            }
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
    EditorScene* activeScene = this->GetActiveScene();
    
    if(activeScene){
        Vec2i ChunksSize = activeScene->GetChunkSize();
        if(ChunksSize != Vec2i(0, 0)){
            glm::vec4 outlineColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            Vec2i chunkSizeEnd = ChunksSize * Volume::Chunk::CHUNK_SIZE + Vec2i(Volume::Chunk::CHUNK_SIZE, Volume::Chunk::CHUNK_SIZE);
            std::vector<glm::vec2> points = {
                {Volume::Chunk::CHUNK_SIZE, Volume::Chunk::CHUNK_SIZE},
                {chunkSizeEnd.x, Volume::Chunk::CHUNK_SIZE},
                {chunkSizeEnd.x, chunkSizeEnd.y},
                {Volume::Chunk::CHUNK_SIZE, chunkSizeEnd.y}
            };
            GameEngine::renderer->DrawClosedShape(points, outlineColor, voxelProjection, 1.0f);
        }
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

    this->imguiRenderer.ActOnFileBrowserSelection();
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
    if(isInDefaultScene){
        isInDefaultScene = false;
        delete oldMatrix;
    }

    size_t index = 0;
    for(size_t i = 0; i < this->scenes.size(); ++i){
        if(this->scenes[i].chunkMatrix == newMatrix){
            index = i;
            break;
        }
    }
    EditorScene *scene = &this->scenes[index];

    // if switching from sandbox, stop some stuff
    if(this->stateStorage.selectedSceneType == EditorScene::Type::Sandbox){
        Editor::instance.SetSimulationActive(false);
        GameEngine::renderer->debugRendering = false;
        GameEngine::renderer->renderMeshData = false;
    }

    this->stateStorage.selectedSceneType = scene->GetType();
    this->activeSceneIndex = index;
}