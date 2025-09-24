#pragma once

#include <cstdint>
#include <string>
#include <array>

class ImGuiRenderer
{
public:
    unsigned short int panelBottomHeight = 150;
    unsigned short int panelSideWidth = 200;
    void RenderPanel();
private:
    uint16_t currentlySelectedTypeIndex = 0;
    std::array<std::string, 2> sceneTypeNames = {
        "Object Editor",
        "Sandbox"
    };

    void RenderEmptyBottomPanel();
    void RenderDrawPanel();
    void RenderLeftPanel();
};