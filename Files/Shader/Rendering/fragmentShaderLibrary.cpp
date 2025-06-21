#include "Shader/Rendering/fragmentShaderLibrary.h"

namespace Shader {
const char* voxelArraySimulationFragmentShader = R"glsl(
#version 460 core
in vec4 vertexColor;    // input color from vertex shader
out vec4 FragColor;     // output color

void main()
{
    FragColor = vertexColor;
}
)glsl";
}