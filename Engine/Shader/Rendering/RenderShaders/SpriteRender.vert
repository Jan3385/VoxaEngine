layout (location = 0) in vec4 quadData;   // vec2 Quad vertex & vec2 Texture coordinates

uniform mat4 projection;                     // Projection matrix
uniform mat4 model;                          // Model matrix

layout(location = 0) out vec2 TexCoords;    // output texture coordinates to fragment shader
void main()
{
    gl_Position = projection * model * vec4(quadData.xy, 0.0, 1.0);
    TexCoords = quadData.zw;                  // Pass texture coordinates to fragment shader
}