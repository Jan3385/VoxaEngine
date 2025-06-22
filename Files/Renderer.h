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

    Shader::Shader chunkRenderProgram;    
    Shader::Shader particleRenderProgram;    

    void ToggleDebugRendering();

    void UpdateParticleVBO(ChunkMatrix &chunkMatrix);

    GLuint particleVAO = 0;
    GLuint particleVBO = 0;
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext);
    ~GameRenderer();

    bool showHeatAroundCursor = false;
    bool debugRendering = false;

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos);
    void RenderIMGUI(ChunkMatrix &chunkMatrix);

    // Chunks which dont have any VBO & VAO yet
    std::vector<Volume::Chunk*> chunkCreateBuffer;

    GLuint quadVBO;		// Predefined quad buffer
};