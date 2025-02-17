#pragma once

#include "World/Chunk.h"
#include "Math/Vector.h"

class GameRenderer{
private:
    SDL_Window *r_window = nullptr;
    SDL_Renderer *r_renderer = nullptr;

    TTF_Font* basicFont;

    void ToggleDebugRendering();
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext);
    ~GameRenderer();

    bool showHeatAroundCursor = false;
    bool debugRendering = false;

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos);
    void RenderIMGUI(ChunkMatrix &chunkMatrix);

    SDL_Texture* LoadTexture(const char* path);
    SDL_Surface* LoadSurface(const char* path);
};