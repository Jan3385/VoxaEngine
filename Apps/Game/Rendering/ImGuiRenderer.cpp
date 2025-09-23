#include "ImGuiRenderer.h"

#include "Game.h"
#include <GameEngine.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "Input/InputHandler.h"

bool ImGuiRenderer::fullImGui = false;

void ImGuiRenderer::RenderDebugPanel()
{
    constexpr int ITEM_WIDTH = 150;

    ImGui::Begin("Game Debug Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    if(!ImGuiRenderer::fullImGui){
        ImGui::Text("Press F1 to expand panel");

        ImGui::End();
        return;
    }

    ImGui::Text("Press F1 to hide panel");
    ImGui::Text("FPS: %lf", GameEngine::instance->FPS);
    ImGui::Text("LAST %d FPS AVG: %lf", AVG_FPS_SIZE_COUNT, GameEngine::instance->avgFPS);
    
    const char* voxelTypeNames[] = {
        "Dirt", "Grass", "Stone", "Sand", "Oxygen",
        "Water", "Fire", "Plasma", "Carbon_Dioxide", "Iron", "Rust", "Wood", "Empty", "Uncarium", "Copper"
    };
    // Find the index of placeVoxelType in voxelTypeNames
    static int currentItem = 0;
    for (int i = 0; i < IM_ARRAYSIZE(voxelTypeNames); ++i) {
        if (voxelTypeNames[i] == Input::mouseData.placeVoxelType) {
            currentItem = i;
            break;
        }
    }

    ImGui::SetNextItemWidth(ITEM_WIDTH);
    if (ImGui::BeginCombo("Placement Voxel", voxelTypeNames[currentItem]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(voxelTypeNames); ++i)
        {
            bool isSelected = (currentItem == i);
            if (ImGui::Selectable(voxelTypeNames[i], isSelected))
            {
                currentItem = i;
                Input::mouseData.placeVoxelType = voxelTypeNames[i];
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::SliderInt("Placement Radius", &Input::mouseData.placementRadius, 0, 10);
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::DragFloat("Placement Temperature", &Input::mouseData.placeVoxelTemperature, 0.5f, -200.0f, 2500.0f);
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::DragInt("Placement Amount", &Input::mouseData.placeVoxelAmount, 10, 1, 2000);
    ImGui::Checkbox("Place Unmovable Solid Voxels", &Input::mouseData.placeUnmovableSolidVoxels);
    if(ImGui::Button("Toggle Debug Rendering")) GameEngine::renderer->ToggleDebugRendering();
    ImGui::Checkbox("Render Mesh Data", &GameEngine::renderer->renderMeshData);
    ImGui::Checkbox("Show Heat Around Cursor", &GameEngine::renderer->showHeatAroundCursor);
    
    if(ImGui::Button("Toggle NoClip")) Game::player->SetNoClip(!Game::player->GetNoClip());

    ImGui::Checkbox("Heat Simulation", &GameEngine::instance->runHeatSimulation);
    ImGui::Checkbox("Pressure Simulation", &GameEngine::instance->runPressureSimulation);
    ImGui::Checkbox("Chemical Simulation", &GameEngine::instance->runChemicalReactions);
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::DragFloat("Heat sim speed", &GameEngine::instance->fixedDeltaTime, 0.05f, 1/30.0, 4);
    ImGui::SetNextItemWidth(ITEM_WIDTH);
    ImGui::DragFloat("CA sim speed", &GameEngine::instance->voxelFixedDeltaTime, 0.05f, 1/30.0, 4);

    ImGui::Text("Loaded chunks: %lld", GameEngine::instance->GetActiveChunkMatrix()->Grid.size());

    ImGui::Checkbox("Player Gun", &Game::player->gunEnabled);
    ImGui::End();
}