#include "SpriteRenderer.h"

#include <SDL.h>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "GameObject/VoxelObject.h"

SpriteRenderer::SpriteRenderer()
{
}

SpriteRenderer::~SpriteRenderer()
{
    if(!initialized)
        return;

    initialized = false;
}
void SpriteRenderer::Initialize()
{
    if(initialized)
        return;

    spriteRenderProgram = new Shader::RenderShader(
        "SpriteRender"
    );

    glm::vec4 quadVertices[] = {
        // Positions        // Texture Coords
        {-0.5f, -0.5f,      0.0f, 0.0f},
        {0.5f, -0.5f,       1.0f, 0.0f},
        {0.5f,  0.5f,       1.0f, 1.0f},
        {-0.5f,  0.5f,      0.0f, 1.0f}
    };
    quadVAO = Shader::GLVertexArray("Sprite Render VAO");
    quadVBO = Shader::GLBuffer<glm::vec4, GL_ARRAY_BUFFER>("Sprite Render VBO");
    quadVBO.SetData(quadVertices, 4, GL_STATIC_DRAW);

    quadVAO.Bind();

    quadVAO.AddAttribute<glm::vec4>(0, 4, quadVBO, GL_FALSE, 0, 0); // location 0: vec4 vertex
    
    quadVAO.Unbind();

    spriteRenderProgram->Use();
    spriteRenderProgram->SetInt("textureID", 0); // Set the texture sampler to use texture unit 0

    initialized = true;
}

// rotation in degrees
void SpriteRenderer::RenderSprite(Vec2f position, Vec2i size, float rotation, GLuint texture, const glm::vec4 &tint, glm::mat4 projection)
{
    if(!initialized)
        return;

    spriteRenderProgram->Use();
    spriteRenderProgram->SetMat4("projection", projection);
    spriteRenderProgram->SetVec4("tint", tint);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));
    model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

    spriteRenderProgram->SetMat4("model", model);

    quadVAO.Bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    quadVAO.Unbind();
}

/// @brief Takes in a .bmp file path and loads the texture into OpenGL GLuint
/// @param filePath 
/// @return GLuint texture ID
GLuint SpriteRenderer::LoadTextureFromFile(const std::string &filePath, Vec2i *r_size)
{
    SDL_Surface *surface = SDL_LoadBMP(filePath.c_str());
    if (!surface) {
        std::cerr << "Failed to load texture from file: " << filePath << " - " << SDL_GetError() << std::endl;
        return 0;
    }

    r_size->x = surface->w;
    r_size->y = surface->h;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, surface->w, surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surface->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    SDL_FreeSurface(surface);

    return textureID;
}
