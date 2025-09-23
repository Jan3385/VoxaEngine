#pragma once

class ImGuiRenderer
{
public:
    unsigned short int panelBottomHeight = 150;
    unsigned short int panelSideWidth = 200;
    void RenderPanel();
private:
    void RenderDrawPanel();
    void RenderLeftPanel();
};