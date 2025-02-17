#pragma once

#include <glew.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "World/Chunk.h"
#include "Math/Vector.h"

class GameRenderer{
private:
    SDL_Window *SDL_window = nullptr;
    SDL_Renderer *SDL_renderer = nullptr;

    TTF_Font* basicFont;
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext);
    ~GameRenderer();

    bool showHeatAroundCursor = false;
    bool debugRendering = false;

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos);
    void RenderIMGUI();
};