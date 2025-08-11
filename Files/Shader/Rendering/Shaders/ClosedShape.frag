#version 460 core

uniform vec4 uColor; // RGBA

out vec4 FragColor;

void main()
{
    FragColor = uColor;
}