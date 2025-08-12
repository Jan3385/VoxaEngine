#pragma once

#include "Shader/Rendering/RenderingShader.h"
#include "Math/Vector.h"

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <string>

struct Character {
    GLuint TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

class Font{
public:
    Font();
    Font(const char* fontPath, FT_Library ft);
    ~Font();

    // Make class uncopyable
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(Font&&) = delete;

    std::map<char, Character> characters;
};

class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    // Make class uncopyable
    FontRenderer(const FontRenderer&) = delete;
    FontRenderer& operator=(const FontRenderer&) = delete;
    FontRenderer(FontRenderer&&) = delete;
    FontRenderer& operator=(FontRenderer&&) = delete;

    void Initialize();

    void RenderText(const std::string &text, Font *font, Vec2f pos, 
        float scale, const glm::vec4 &color, glm::mat4 projection);

    Font* pixelFont;
private:
    bool initialized = false;
    Shader::RenderShader *textRenderProgram = nullptr;
    GLuint VAO, VBO;
};