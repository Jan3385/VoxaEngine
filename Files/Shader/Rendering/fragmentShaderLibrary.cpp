#include "Shader/Rendering/fragmentShaderLibrary.h"

namespace Shader {
const char* voxelArraySimulationFragmentShader = R"glsl(
#version 460 core
out vec4 FragColor;     // output color
in vec3 vertexColor;    // input color from vertex shader

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
)glsl";
}