#include "SpriteRenderer.h"

#include <SDL.h>
#include <iostream>

#include "Shader/Rendering/vertexShaderLibrary.h"
#include "Shader/Rendering/fragmentShaderLibrary.h"
#include <glm/gtc/matrix_transform.hpp>

#include "GameObject/GameObject.h"

SpriteRenderer::SpriteRenderer()
{
}

SpriteRenderer::~SpriteRenderer()
{
    if(!initialized)
        return;

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    initialized = false;
}
void SpriteRenderer::Initialize()
{
    if(initialized)
        return;

    spriteRenderProgram = Shader::Shader(
        Shader::spriteRenderVertexShader,
        Shader::spriteRenderFragmentShader
    );

    float quadVertices[] = {
        // Positions    // Texture Coords
        -0.5f, -0.5f,   0.0f, 0.0f,
         0.5f, -0.5f,   1.0f, 0.0f,
         0.5f,  0.5f,   1.0f, 1.0f,
        -0.5f,  0.5f,   0.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    spriteRenderProgram.Use();
    spriteRenderProgram.SetInt("textureID", 0); // Set the texture sampler to use texture unit 0

    initialized = true;
}

// rotation in degrees
void SpriteRenderer::RenderSprite(GameObject *object, float rotation, const glm::vec4 &tint, glm::mat4 projection)
{
    if(!initialized)
        return;

    spriteRenderProgram.Use();
    spriteRenderProgram.SetMat4("projection", projection);
    spriteRenderProgram.SetVec4("tint", tint);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, object->GetTexture());

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(object->GetPosition().getX(), object->GetPosition().getY(), 0.0f));
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(object->GetSize().getX(), object->GetSize().getY(), 1.0f));
    
    spriteRenderProgram.SetMat4("model", model);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
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

    r_size->x(surface->w);
    r_size->y(surface->h);

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
