#include "ImGuiRenderer.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <GameEngine.h>

void ImGuiRenderer::RenderPanel()
{
    ImGui::Begin("Debug panel");
    
    if(ImGui::Button("Spawn (1,1) chunk")){
        GameEngine::instance->LoadChunkAtPosition(Vec2i(1, 1));
    }

    ImGui::End();
}