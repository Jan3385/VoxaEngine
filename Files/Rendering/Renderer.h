#pragma once

#include "World/Chunk.h"
#include "Math/Vector.h"
#include "Shader/Rendering/RenderingShader.h"
#include "Shader/Rendering/vertexShaderLibrary.h"
#include "Shader/Rendering/fragmentShaderLibrary.h"
#include "Rendering/FontRenderer.h"
#include "Rendering/SpriteRenderer.h"

class GameRenderer{
private:
    FontRenderer fontRenderer;
    SpriteRenderer spriteRenderer;

    SDL_Window *r_window = nullptr;

    // Borrowed from Engine class
    SDL_GLContext *r_GLContext = nullptr;

    Shader::Shader chunkRenderProgram;    
    Shader::Shader temperatureRenderProgram;
    Shader::Shader particleRenderProgram;    
    Shader::Shader closedShapeRenderProgram;
    
    void ToggleDebugRendering();

    void UpdateParticleVBO(ChunkMatrix &chunkMatrix);

    GLuint particleVAO = 0;
    GLuint particleVBO = 0;

    GLuint closedShapeVAO = 0;
    GLuint closedShapeVBO = 0;
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext);
    ~GameRenderer();

    bool showHeatAroundCursor = false;
    bool debugRendering = false;

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos);
    void DrawClosedShape(const std::vector<glm::vec2> &points, const glm::vec4 &color, glm::mat4 projection, float lineWidth);
    void DrawClosedShape(const GLuint VAO, const GLsizei size, const glm::vec4 &color, glm::mat4 projection, float lineWidth);
    void RenderIMGUI(ChunkMatrix &chunkMatrix);

    // Chunks which dont have any VBO & VAO yet
    std::vector<Volume::Chunk*> chunkCreateBuffer;

    GLuint quadVBO;		// Predefined quad buffer
};