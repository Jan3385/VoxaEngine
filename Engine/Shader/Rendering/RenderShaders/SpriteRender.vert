layout (location = 0) in vec2 quadVertex;   // Quad vertex
layout (location = 1) in vec2 aTexCoord;    // Texture coordinates

uniform mat4 projection;                     // Projection matrix
uniform mat4 model;                          // Model matrix

layout(location = 0) out vec2 TexCoords;    // output texture coordinates to fragment shader
void main()
{
    gl_Position = projection * model * vec4(quadVertex, 0.0, 1.0);
    TexCoords = aTexCoord;                  // Pass texture coordinates to fragment shader
}