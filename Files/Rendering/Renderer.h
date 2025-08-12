#pragma once

#include "World/Chunk.h"
#include "Math/Vector.h"
#include "GameObject/Player.h"
#include "Shader/Rendering/RenderingShader.h"
#include "Rendering/FontRenderer.h"
#include "Rendering/SpriteRenderer.h"

class GameRenderer{
private:
    FontRenderer fontRenderer;
    SpriteRenderer spriteRenderer;

    SDL_Window *r_window = nullptr;

    // Borrowed from Engine class
    SDL_GLContext *r_GLContext = nullptr;

    Shader::RenderShader *voxelRenderProgram = nullptr;    
    Shader::RenderShader *temperatureRenderProgram = nullptr;
    Shader::RenderShader *particleRenderProgram = nullptr;    
    Shader::RenderShader *closedShapeRenderProgram = nullptr;
    Shader::RenderShader *cursorRenderProgram = nullptr;

    void ToggleDebugRendering();

    void RenderVoxelObjects(ChunkMatrix &chunkMatrix, glm::mat4 projection);
    void RenderPlayer(Game::Player *player, glm::mat4 projection);
    void RenderChunks(ChunkMatrix &chunkMatrix, Game::Player *player, glm::mat4 projection);
    void RenderHeat(ChunkMatrix &chunkMatrix, glm::vec2 mousePos, Game::Player *player, glm::mat4 projection);
    void RenderParticles(ChunkMatrix &chunkMatrix, glm::mat4 projection);
    void RenderCursor(glm::vec2 mousePos, Game::Player *player, glm::mat4 projection);

    void RenderDebugMode(ChunkMatrix &chunkMatrix, Game::Player *player, glm::vec2 mousePos, glm::mat4 voxelProj, glm::mat4 screenProj);
    void RenderMeshData(ChunkMatrix &chunkMatrix, Game::Player *player, glm::mat4 projection);
    
    void RenderIMGUI(ChunkMatrix &chunkMatrix, Game::Player *player);

    void UpdateParticleVBO(ChunkMatrix &chunkMatrix);

    GLuint particleVAO = 0;
    GLuint particleVBO = 0;

    GLuint closedShapeVAO = 0;
    GLuint closedShapeVBO = 0;

    GLuint cursorVAO = 0;
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext);
    ~GameRenderer();

    bool showHeatAroundCursor = false;
    bool debugRendering = false;
    bool renderMeshData = false;

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos);
    void DrawClosedShape(const std::vector<glm::vec2> &points, const glm::vec4 &color, glm::mat4 projection, float lineWidth);
    void DrawClosedShape(const GLuint VAO, const GLsizei size, const glm::vec4 &color, glm::mat4 projection, float lineWidth);

    // Chunks which dont have any VBO & VAO yet
    std::vector<Volume::Chunk*> chunkCreateBuffer;

    GLuint quadVBO;		// Predefined quad buffer
};