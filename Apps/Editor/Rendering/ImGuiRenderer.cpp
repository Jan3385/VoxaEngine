#include "ImGuiRenderer.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <GameEngine.h>

#include "Input/InputHandler.h"

void ImGuiRenderer::RenderPanel()
{
    ImGui::Begin("Debug panel");
    
    if(ImGui::Button("Spawn (1,1) chunk")){
        GameEngine::instance->LoadChunkAtPosition(Vec2i(1, 1));
    }

    ImGui::End();

    this->RenderDrawPanel();
}

void ImGuiRenderer::RenderDrawPanel()
{
    Vec2i screenCorner = GameEngine::instance->WindowSize;

    ImGui::Begin("Draw Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowPos(ImVec2(0, screenCorner.y - panelBottomHeight));
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
