#version 460 core
layout (location = 0) in vec2 quadVertex;          // Quad vertex (0-1)
layout (location = 1) in ivec2 instancePos;  // Voxel position
layout (location = 2) in vec4 instanceColor;

uniform mat4 projection;                     // Projection matrix

out vec4 vertexColor;                        // output color to fragment shader

void main()
{
    gl_Position = projection * vec4(instancePos + quadVertex, 0.0, 1.0);

    vertexColor = instanceColor;
}