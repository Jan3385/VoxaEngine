#include "Shader/Rendering/fragmentShaderLibrary.h"

namespace Shader {
// ---------------------------------------------
// Fragment shaders for rendering entire chunks
// ---------------------------------------------
const char* voxelArraySimulationFragmentShader = R"glsl(
#version 460 core
in vec4 vertexColor;    // input color from vertex shader

out vec4 FragColor;     // output color

void main()
{
    FragColor = vertexColor;
}
)glsl";

// ---------------------------------------------
// Fragment shaders for rendering particles
// ---------------------------------------------
const char* voxelParticleFragmentShader = R"glsl(
#version 460 core
in vec4 vertexColor;    // input color from vertex shader

out vec4 FragColor;     // output color

void main()
{
    FragColor = vertexColor;
}
)glsl";

// ---------------------------------------------
// Fragment shaders for rendering closed shapes
// ---------------------------------------------
const char* closedShapeDrawFragmentShader = R"glsl(
#version 460 core

uniform vec4 uColor; // RGBA

out vec4 FragColor;

void main()
{
    FragColor = uColor;
}
)glsl";

// ---------------------------------------------
// Fragment shaders for rendering text
// ---------------------------------------------
const char* textRenderFragmentShader = R"glsl(
#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text; // Texture containing the character glyph
uniform vec3 textColor; // RGB color for the text
void main()
{
    float alpha = texture(text, TexCoords).r;
    FragColor = texture(text, TexCoords) + vec4(0.0, 0.0, 0.0, 1.0);
}
)glsl";

}