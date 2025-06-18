#pragma once

#include "World/Chunk.h"
#include "Math/Vector.h"
#include "Shader/Rendering/RenderingShader.h"
#include "Shader/Rendering/vertexShaderLibrary.h"
#include "Shader/Rendering/fragmentShaderLibrary.h"

class GameRenderer{
private:
    SDL_Window *r_window = nullptr;
    // Borrowed from Engine class
    SDL_GLContext *r_GLContext = nullptr;

    Shader::Shader voxelRenderProgram;    
    GLuint voxelArrayVAO = 0;
    GLuint voxelArrayVBO = 0;

    TTF_Font* basicFont;

    void ToggleDebugRendering();
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext);
    ~GameRenderer();

    bool showHeatAroundCursor = false;
    bool debugRendering = false;

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos);
    //void RenderIMGUI(ChunkMatrix &chunkMatrix);

};