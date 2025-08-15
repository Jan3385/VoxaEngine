layout(location = 0) in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D textureID;
uniform vec4 tint;

void main()
{
    FragColor = texture(textureID, TexCoords);
    FragColor *= tint; // Apply tint to the texture color
}