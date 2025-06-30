#pragma once

#include "Shader/Rendering/RenderingShader.h"
#include "Math/Vector.h"

#include <glew.h>
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

    std::map<char, Character> characters;
};

class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    void Initialize();

    void RenderText(const std::string &text, Vec2f pos, 
        float scale, const glm::vec3 &color, glm::mat4 projection);

    Font pixelFont;
private:
    Shader::Shader textRenderProgram;
    GLuint VAO, VBO;
};