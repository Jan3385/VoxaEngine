#include "Shader/Rendering/vertexShaderLibrary.h"

namespace Shader {

const char* voxelArraySimulationVertexShader = R"glsl(
#version 460 core
layout (location = 0) in vec2 aPos;         // Quad vertex (0-1)
layout (location = 1) in vec2 instancePos;  // Voxel position
layout (location = 2) in vec4 instanceColor;

uniform mat4 projection;                    // Projection matrix

out vec4 vertexColor;                       // output color to fragment shader

void main()
{
    vec2 scaledPos = aPos * vec2(4.0); // 4x4px voxel
    gl_Position = projection * vec4(instancePos + scaledPos, 0.0, 1.0);
    vertexColor = instanceColor;
}
)glsl";


}