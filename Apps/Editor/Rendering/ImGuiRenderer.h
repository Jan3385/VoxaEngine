#pragma once

class ImGuiRenderer
{
public:
    void RenderPanel();
private:
    unsigned short int panelBottomHeight = 150;
    unsigned short int panelSideWidth = 200;
    void RenderDrawPanel();
};