#version 460 core
layout(location = 0) in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D textureID; // Texture containing the character glyph
uniform vec4 textColor; // RGBA color for the text
void main()
{
    float alpha = texture(textureID, TexCoords).r;
    alpha = alpha * textColor.a;
    FragColor = vec4(textColor.rgb, alpha);
}