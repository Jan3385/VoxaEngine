#include "Shader/Rendering/vertexShaderLibrary.h"

namespace Shader {

const char* voxelArraySimulationVertexShader = R"glsl(
#version 460 core
layout(location = 0) in vec2 aPos;      // position attribute
layout(location = 1) in vec4 aColor;    // color attribute

out vec3 vertexColor;                   // output color to fragment shader

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}
)glsl";


}