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
// Fragment shaders for rendering temperature
// ---------------------------------------------
const char* temperatureVoxelFragmentShader = R"glsl(
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

uniform sampler2D textureID; // Texture containing the character glyph
uniform vec4 textColor; // RGBA color for the text
void main()
{
    float alpha = texture(textureID, TexCoords).r;
    alpha = alpha * textColor.a;
    FragColor = vec4(textColor.rgb, alpha);
}
)glsl";

// ---------------------------------------------
// Fragment shaders for rendering sprites
// ---------------------------------------------
const char* spriteRenderFragmentShader = R"glsl(
#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D textureID;
uniform vec4 tint;

void main()
{
    FragColor = texture(textureID, TexCoords);
    FragColor *= tint; // Apply tint to the texture color
}

)glsl";

// ---------------------------------------------
// Fragment shaders for rendering cursors
// ---------------------------------------------
const char* cursorRenderFragmentShader = R"glsl(
#version 460 core
#define VOXEL_PIXEL_SIZE 4

in vec2 uv;
out vec4 FragColor;

uniform vec2 size;
uniform vec4 outlineColor;

void main()
{
    float borderThickness = 1;

    vec2 thickness = borderThickness / (size * vec2(VOXEL_PIXEL_SIZE));
    vec2 extraThickness = vec2(1.0) / (size * vec2(VOXEL_PIXEL_SIZE));

    float dist = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));

    if (dist < thickness.x + extraThickness.x || dist < thickness.y + extraThickness.y) {
        // slight border falloff
        float alpha = dist < thickness.x ? 1.0 : 0.25;

        FragColor = vec4(outlineColor.rgb, alpha);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}

)glsl";

}