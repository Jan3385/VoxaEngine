#pragma once

#include "World/Chunk.h"
#include "Math/Vector.h"
#include "Shader/Rendering/RenderingShader.h"
#include "Rendering/FontRenderer.h"
#include "Rendering/SpriteRenderer.h"

struct IGame;

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

    void RenderVoxelObjects(ChunkMatrix &chunkMatrix, glm::mat4 projection);
    void RenderPlayer(VoxelObject *player, glm::mat4 projection);
    void RenderChunks(ChunkMatrix &chunkMatrix, glm::mat4 projection);
    void RenderHeat(ChunkMatrix &chunkMatrix, glm::vec2 mousePos, glm::mat4 projection);
    void RenderParticles(ChunkMatrix &chunkMatrix, glm::mat4 projection);

    void RenderDebugMode(ChunkMatrix &chunkMatrix, glm::vec2 mousePos, glm::mat4 voxelProj, glm::mat4 screenProj);
    void RenderMeshData(ChunkMatrix &chunkMatrix, glm::mat4 projection);

    void UpdateParticleVBO(ChunkMatrix &chunkMatrix);

    AABB Camera;

    Shader::GLVertexArray particleVAO;
    Shader::GLBuffer<Particle::ParticleRenderData, GL_ARRAY_BUFFER> particleVBO;

    Shader::GLVertexArray closedShapeVAO;
    Shader::GLBuffer<glm::vec2, GL_ARRAY_BUFFER> closedShapeVBO;

    Shader::GLVertexArray cursorVAO;
public:
    GameRenderer();
    GameRenderer(SDL_GLContext *glContext, RGB backgroundColor);
    ~GameRenderer();

    RGB backgroundColor;

    bool showHeatAroundCursor = false;
    bool debugRendering = false;
    bool renderMeshData = false;

    void SetCameraPosition(Vec2f centerPos);
    void SetCameraSize(Vec2f size);
    Vec2f GetCameraOffset() const { return this->Camera.corner; }
    AABB GetCameraAABB() const { return this->Camera; }

    void SetVSYNC(bool enabled);

    void RenderCursor(glm::vec2 mousePos, glm::mat4 projection, int cursorSize);

    void ToggleDebugRendering();

    void Render(ChunkMatrix &chunkMatrix, Vec2i mousePos, IGame *game);
    void DrawClosedShape(const std::vector<glm::vec2> &points, const glm::vec4 &color, glm::mat4 projection, float lineWidth);
    void DrawClosedShape(const GLuint VAO, const GLsizei size, const glm::vec4 &color, glm::mat4 projection, float lineWidth);

    Shader::GLBuffer<glm::vec2, GL_ARRAY_BUFFER> *quadBuffer;
};