#version 460 core

layout (location = 0) in vec2 vert;

uniform mat4 uProjection;

void main()
{
    gl_Position = uProjection * vec4(vert, 0.0, 1.0);
}