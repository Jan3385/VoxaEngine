#pragma once

#include <cstdint>
#include <string>
#include <array>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include "External/imfilebrowser.h"

class ImGuiRenderer
{
public:
    ImGuiRenderer();
    unsigned short int panelBottomHeight = 150;
    unsigned short int panelSideWidth = 200;
    void RenderPanel();
    void ActOnFileBrowserSelection();
private:
    ImGui::FileBrowser fileDialog;

    uint16_t currentlySelectedTypeIndex = 0;
    std::array<std::string, 2> sceneTypeNames = {
        "Object Editor",
        "Sandbox"
    };
    std::array<std::string, 3> voxelStateNames = {
        "Gas",
        "Liquid",
        "Solid"
    };

    void RenderEmptyBottomPanel();
    void RenderDrawPanel();
    void RenderSandboxPanel();
    void RenderLeftPanel();
};