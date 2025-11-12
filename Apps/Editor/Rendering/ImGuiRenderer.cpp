#include "ImGuiRenderer.h"

#include <GameEngine.h>

#include "Editor.h"
#include "Input/InputHandler.h"
#include "Generation/ChunkGenerator.h"
#include "Generation/VoxelObjLoader.h"

ImGuiRenderer::ImGuiRenderer()
    : fileDialog(ImGuiFileBrowserFlags_ConfirmOnEnter, std::filesystem::current_path())
{
    this->fileDialog.SetTitle("Select Voxel File to load");
    this->fileDialog.SetTypeFilters({ ".bmp", ".BMP" });
}

void ImGuiRenderer::RenderPanel()
{
    this->RenderLeftPanel();

    EditorScene *activeScene = Editor::instance.GetActiveScene();

    if(!activeScene) this->RenderEmptyBottomPanel();
    else switch(activeScene->GetType()){
        case EditorScene::Type::ObjectEditor:
            this->RenderDrawPanel();
            break;
        case EditorScene::Type::Sandbox:
            this->RenderSandboxPanel();
            break;
        default:
            this->RenderEmptyBottomPanel();
            break;
    }
    this->fileDialog.Display();
}

void ImGuiRenderer::ActOnFileBrowserSelection()
{
    if(!this->fileDialog.HasSelected()) return;

    std::string selectedFilePath = this->fileDialog.GetSelected().string();
    EditorScene *activeScene = Editor::instance.GetActiveScene();

    if(activeScene && !selectedFilePath.empty()){
        Debug::LogInfo("Selected voxel file: " + selectedFilePath);
        ObjLoader::InsertVoxelsFromFileIntoScene(selectedFilePath, activeScene);
    }

    this->fileDialog.ClearSelected();
}

void ImGuiRenderer::RenderEmptyBottomPanel()
{
    Vec2i screenCorner = GameEngine::instance->WindowSize;

    ImGui::Begin("Bottom Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowPos(ImVec2(panelSideWidth, screenCorner.y - panelBottomHeight));
    ImGui::SetWindowSize(ImVec2(screenCorner.x, panelBottomHeight));

    ImGui::End();
}

void ImGuiRenderer::RenderDrawPanel()
{
    Vec2i screenCorner = GameEngine::instance->WindowSize;

    ImGui::Begin("Draw Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowPos(ImVec2(panelSideWidth, screenCorner.y - panelBottomHeight));
    ImGui::SetWindowSize(ImVec2(screenCorner.x, panelBottomHeight));

    ImGui::BeginGroup();
    std::array<float, 4> color = Input::mouseData.placeColor.toFloatArray();
    ImGui::SetNextItemWidth(panelBottomHeight * 0.75f - 1);
    ImGui::ColorPicker4("Color", color.data(), ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_DisplayRGB);
    Input::mouseData.placeColor = RGBA(color);
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Draw Options");
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Brush Radius", &Input::mouseData.brushRadius, 0, 10);
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Checkbox("Load color not material", &Editor::instance.stateStorage.loadColorFromBMP);
    if(ImGui::Button("Load Voxel File"))
        this->fileDialog.Open();
    ImGui::EndGroup();

    ImGui::End();
}

void ImGuiRenderer::RenderSandboxPanel()
{
    Vec2i screenCorner = GameEngine::instance->WindowSize;

    ImGui::Begin("Sandbox Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowPos(ImVec2(panelSideWidth, screenCorner.y - panelBottomHeight));
    ImGui::SetWindowSize(ImVec2(screenCorner.x, panelBottomHeight));

    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(panelBottomHeight * 0.75f - 1);
    std::string currentStateName = this->voxelStateNames[static_cast<int>(Editor::instance.stateStorage.selectedVoxelState)];
    if(ImGui::BeginCombo("State", currentStateName.c_str())){
        for(size_t i = 0; i < this->voxelStateNames.size(); i++){
            bool isSelected = (i == static_cast<size_t>(Editor::instance.stateStorage.selectedVoxelState));

            if(ImGui::Selectable(this->voxelStateNames[i].c_str(), isSelected)){
                Editor::instance.stateStorage.selectedVoxelState = static_cast<Volume::State>(i);
            }
                
            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if(Editor::instance.stateStorage.selectedVoxelState == Volume::State::Solid)
        ImGui::Checkbox("Place Unmovable", &Editor::instance.stateStorage.placedSimVoxelsAreUnmovable);
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Text("Simulation Options");
    if(ImGui::Button("Start Simulation")) {
        Editor::instance.SetSimulationActive(true);
    }
    if(ImGui::Button("Pause Simulation")){
        Editor::instance.SetSimulationActive(false);
    }
    ImGui::SetNextItemWidth(panelBottomHeight * 0.75f - 1);
    if(ImGui::SliderFloat("Simulation FPS", &Editor::instance.stateStorage.simulationFPS, 0.0f, 30.0f, "%.1f FPS")){
        GameEngine::instance->voxelFixedDeltaTime = 1.0f / Editor::instance.stateStorage.simulationFPS;

        // Stop simulation if FPS is set to 0
        if(GameEngine::instance->voxelFixedDeltaTime <= 0.0f)
            Editor::instance.SetSimulationActive(false);
    }
    if(ImGui::Button("Step")){
        // Ignore the step if the simulation is running
        if(!Editor::instance.stateStorage.runSimulationAuto)
            GameEngine::instance->VoxelSimulationStep();
    }
    ImGui::EndGroup();
    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::Checkbox("Debug Rendering", &GameEngine::instance->renderer->debugRendering);
    ImGui::Checkbox("Render Mesh Data", &GameEngine::renderer->renderMeshData);
    ImGui::EndGroup();

    ImGui::End();
}

void ImGuiRenderer::RenderLeftPanel()
{
    Vec2i screenCorner = GameEngine::instance->WindowSize;

    ImGui::Begin("Left Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(panelSideWidth, screenCorner.y));

    ImGui::Text("Editor options");
    
    // generating new chunks
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(83, 104, 120, 255));
    ImGui::BeginChild("Chunk Matrix", ImVec2(panelSideWidth - 16, 130), false);
    ImGui::Spacing();

    std::string currentSceneName = Editor::instance.scenes.empty() ? "No scene" : Editor::instance.scenes[Editor::instance.activeSceneIndex].name;
    if(ImGui::BeginCombo("Scene", currentSceneName.c_str())){
        for(size_t i = 0; i < Editor::instance.scenes.size(); i++){
            bool isSelected = (i == Editor::instance.activeSceneIndex);

            if(ImGui::Selectable(Editor::instance.scenes[i].name.c_str(), isSelected)){
                Editor::instance.SwitchToScene(i);
            }
                
            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Text(("(Chunk is " + std::to_string(Volume::Chunk::CHUNK_SIZE) + "x" + std::to_string(Volume::Chunk::CHUNK_SIZE) + " voxels)").c_str());

    ImGui::Text("Type of new scenes");
    std::string currentTypeName = this->sceneTypeNames[this->currentlySelectedTypeIndex];
    if(ImGui::BeginCombo("Type", currentTypeName.c_str())){
        for(size_t i = 0; i < this->sceneTypeNames.size(); i++){
            bool isSelected = (i == this->currentlySelectedTypeIndex);

            if(ImGui::Selectable(this->sceneTypeNames[i].c_str(), isSelected)){
                this->currentlySelectedTypeIndex = i;
                Editor::instance.stateStorage.selectedSceneType = static_cast<EditorScene::Type>(i + 1);
            }
                
            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }


    ImGui::SetNextItemWidth(panelSideWidth * 0.45f);
    ImGui::DragInt2("Size", reinterpret_cast<int*>(&Editor::instance.stateStorage.generateNewChunksSize.x), 0.1f, 1, 16, "%d", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SameLine();
    if(ImGui::Button("Create"))
        Generator::SetNewMatrix(Editor::instance.stateStorage.generateNewChunksSize, Editor::instance.stateStorage.selectedSceneType);
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("Generates a *new chunk matrix* of specified size");

    if(ImGui::Button("Expand to size", ImVec2(panelSideWidth - 20, 0)))
        Generator::ExpandMatrixToSize(Editor::instance.stateStorage.generateNewChunksSize);
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("*Expands* an existing chunk matrix to the specified size");

    ImGui::Spacing();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
}
