layout (location = 0) in vec2 quadVertex;   // Quad vertex (0-1)

uniform vec2 size;
uniform vec2 cursorPosition;
uniform mat4 projection;

layout(location = 0) out vec2 uv;

void main()
{
    uv = quadVertex;

    vec2 halfSize = floor(size / 2.0);
    vec2 vertex = (quadVertex * size - halfSize + cursorPosition);
    gl_Position = projection * vec4(vertex, 0.0, 1.0);
}