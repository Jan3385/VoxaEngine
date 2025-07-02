#pragma once

#include "Shader/Rendering/RenderingShader.h"
#include "Math/Vector.h"

#include <glew.h>
#include <glm/glm.hpp>

class GameObject;

class SpriteRenderer
{
private:
    bool initialized = false;

    GLuint quadVAO;
    GLuint quadVBO;

    Shader::Shader spriteRenderProgram;
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // Make class uncopyable
    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;
    SpriteRenderer(SpriteRenderer&&) = delete;
    SpriteRenderer& operator=(SpriteRenderer&&) = delete;

    void Initialize();
    void RenderSprite(GameObject *object, float rotation, const glm::vec4& tint, glm::mat4 projection);

    static GLuint LoadTextureFromFile(const std::string& filePath, Vec2i *r_size);
};