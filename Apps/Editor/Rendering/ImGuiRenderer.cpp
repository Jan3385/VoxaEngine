#include "ImGuiRenderer.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <GameEngine.h>

#include "Editor.h"
#include "Input/InputHandler.h"
#include "Generation/ChunkGenerator.h"

void ImGuiRenderer::RenderPanel()
{
    ImGui::Begin("Debug panel");

    ImGui::End();

    this->RenderLeftPanel();
    this->RenderDrawPanel();
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
    ImGui::BeginChild("Chunk Matrix", ImVec2(panelSideWidth - 10, 70), false);
    ImGui::Text("Generate new matrix");
    ImGui::SetNextItemWidth(panelSideWidth * 0.45f);
    ImGui::DragInt2("Size", reinterpret_cast<int*>(&Editor::instance.stateStorage.generateNewChunksSize), 0.1f, 1, 100);
    ImGui::SameLine();
    if(ImGui::Button("Create"))
        Generator::SetNewMatrix(Editor::instance.stateStorage.generateNewChunksSize);
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("Generates a *new chunk matrix* of specified size");

    if(ImGui::Button("Expand to size", ImVec2(panelSideWidth - 20, 0)))
        Generator::ExpandMatrixToSize(Editor::instance.stateStorage.generateNewChunksSize);
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("*Expands* an existing chunk matrix to the specified size");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Checkbox("Debug Rendering", &GameEngine::instance->renderer->debugRendering);

    ImGui::End();
}
