#include "FontRenderer.h"

#include <stdexcept>
#include <iostream>
#include <vector>

FontRenderer::FontRenderer()
{

}

void FontRenderer::Initialize()
{
    if (initialized)
        return;

    this->textRenderProgram = new Shader::RenderShader(
        "TextRender"
    );

    // Setup VAO and VBO for text rendering
    textVAO = Shader::GLVertexArray("Text Render VAO");

    textVAO.Bind();
    textVBO = Shader::GLBuffer<glm::vec4, GL_ARRAY_BUFFER>("Text Render VBO");
    textVAO.AddAttribute<glm::vec4>(0, 4, textVBO, GL_FALSE, 0, 0); // location 0: vec4 vertex
    textVAO.Unbind();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        throw std::runtime_error("Could not init FreeType Library");
    }

    this->pixelFont = new Font("Fonts/Tiny5/Tiny5-Regular.ttf", ft);

    FT_Done_FreeType(ft);

    initialized = true;
}

FontRenderer::~FontRenderer()
{
    if (!initialized)
        return;

    delete this->pixelFont;
    delete this->textRenderProgram;

    initialized = false;
}

void FontRenderer::RenderText(const std::string &text, Font *font, Vec2f pos, 
    float scale, const glm::vec4 &color, glm::mat4 projection)
{
    this->textRenderProgram->Use();
    this->textRenderProgram->SetVec4("textColor", color);
    this->textRenderProgram->SetMat4("projection", projection);
    this->textRenderProgram->SetInt("textureID", 0);
    glActiveTexture(GL_TEXTURE0);
    textVAO.Bind();

    std::string::const_iterator c;
    for(c = text.begin(); c != text.end(); c++) {
        Character ch = font->characters[*c];

        float xpos = pos.x + ch.Bearing.x * scale;
        float ypos = pos.y + (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        glm::vec4 vertices[6] = {
            { xpos,     ypos - h,   0.0f, 0.0f }, // Top-left
            { xpos,     ypos,       0.0f, 1.0f }, // Bottom-left
            { xpos + w, ypos,       1.0f, 1.0f }, // Bottom-right

            { xpos,     ypos - h,   0.0f, 0.0f }, // Top-left
            { xpos + w, ypos,       1.0f, 1.0f }, // Bottom-right
            { xpos + w, ypos - h,   1.0f, 0.0f }  // Top-right
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        textVBO.SetData(vertices, 6, GL_DYNAMIC_DRAW);
        textVBO.Unbind();

        glDrawArrays(GL_TRIANGLES, 0, 6); // Render the quad

        // Advance the cursor for the next glyph
        pos.x += (ch.Advance >> 6) * scale;
    }
    textVAO.Unbind();
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
}

Font::Font()
{
}

Font::Font(const char* fontPath, FT_Library ft)
{
    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        throw std::runtime_error("Could not load font face");
    }
    FT_Set_Pixel_Sizes(face, 0, 8);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    for(unsigned char c = 0; c < 128; c++){
        // Load character glyph
        if(FT_Load_Char(face, c, FT_LOAD_RENDER)){
            throw std::runtime_error("Could not load character glyph");
        }
        

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED, 
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        this->characters.insert(std::pair<char, Character>(c, character));
        
    }
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    FT_Done_Face(face);
}

Font::~Font()
{
    for (auto& pair : this->characters) {
        glDeleteTextures(1, &pair.second.TextureID);
    }
}
