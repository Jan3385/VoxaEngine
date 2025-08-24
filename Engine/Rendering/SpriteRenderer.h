#pragma once

#include "Shader/Rendering/RenderingShader.h"
#include "Math/Vector.h"
#include "Shader/GLVertexArray.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

class VoxelObject;

class SpriteRenderer
{
private:
    bool initialized = false;

    Shader::GLVertexArray quadVAO;
    Shader::GLBuffer<glm::vec4, GL_ARRAY_BUFFER> quadVBO;

    Shader::RenderShader *spriteRenderProgram = nullptr;
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // Make class uncopyable
    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;
    SpriteRenderer(SpriteRenderer&&) = delete;
    SpriteRenderer& operator=(SpriteRenderer&&) = delete;

    void Initialize();
    void RenderSprite(Vec2f position, Vec2i size, float rotation, GLuint texture, const glm::vec4& tint, glm::mat4 projection);

    static GLuint LoadTextureFromFile(const std::string& filePath, Vec2i *r_size);
};