#include "Shader/Rendering/vertexShaderLibrary.h"

namespace Shader {

// ---------------------------------------------
// Vertex shaders for rendering entire chunks
// ---------------------------------------------
const char* voxelArraySimulationVertexShader = R"glsl(
#version 460 core
layout (location = 0) in vec2 aPos;          // Quad vertex (0-1)
layout (location = 1) in ivec2 instancePos;  // Voxel position
layout (location = 2) in vec4 instanceColor;

uniform mat4 projection;                     // Projection matrix

out vec4 vertexColor;                        // output color to fragment shader

void main()
{
    gl_Position = projection * vec4(instancePos + aPos, 0.0, 1.0);

    vertexColor = instanceColor;
}
)glsl";

// ---------------------------------------------
// Vertex shaders for rendering particles
// ---------------------------------------------
const char* voxelParticleVertexShader = R"glsl(
#version 460 core
layout (location = 0) in vec2 aPos;          // Quad vertex (0-1)
layout (location = 1) in vec2 instancePos;  // Voxel position
layout (location = 2) in vec4 instanceColor;

uniform mat4 projection;                     // Projection matrix

out vec4 vertexColor;                        // output color to fragment shader

void main()
{
    gl_Position = projection * vec4(instancePos + aPos, 0.0, 1.0);

    vertexColor = instanceColor;
}
)glsl";

// ---------------------------------------------
// Vertex shaders for rendering closed shapes
// ---------------------------------------------
const char* closedShapeDrawVertexShader = R"glsl(
#version 460 core

layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)glsl";

// ---------------------------------------------
// Vertex shaders for rendering text
// ---------------------------------------------
const char* textRenderVertexShader = R"glsl(
#version 460 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 text>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)glsl";

}