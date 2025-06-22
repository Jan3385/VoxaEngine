#include "Shader/Rendering/fragmentShaderLibrary.h"

namespace Shader {
const char* voxelArraySimulationFragmentShader = R"glsl(
#version 460 core
in vec4 vertexColor;    // input color from vertex shader
flat in ivec2 instanceLocalPos; // input local position from vertex shader

uniform bool isDebugRendering; // debug rendering flag

out vec4 FragColor;     // output color

void main()
{
    if(!isDebugRendering) {
        FragColor = vertexColor; // use the vertex color directly
        return;
    }

    bool isOnBorder = instanceLocalPos.x < 1.0 || instanceLocalPos.x >= 63.0 ||
                        instanceLocalPos.y < 1.0 || instanceLocalPos.y >= 63.0;

    vec4 red = vec4(1.0, 0.0, 0.0, 1.0);

    vec4 baseColor = isOnBorder ? mix(vertexColor, red, 0.7) : vertexColor;

    FragColor = baseColor;
}
)glsl";
}