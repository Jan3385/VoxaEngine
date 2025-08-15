#define VOXEL_PIXEL_SIZE 4

layout(location = 0) in vec2 uv;
out vec4 FragColor;

uniform vec2 size;
uniform vec4 outlineColor;

void main()
{
    float borderThickness = 1;

    vec2 thickness = borderThickness / (size * vec2(VOXEL_PIXEL_SIZE));
    vec2 extraThickness = vec2(1.0) / (size * vec2(VOXEL_PIXEL_SIZE));

    float dist = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));

    if (dist < thickness.x + extraThickness.x || dist < thickness.y + extraThickness.y) {
        // slight border falloff
        float alpha = dist < thickness.x ? 1.0 : 0.25;

        FragColor = vec4(outlineColor.rgb, alpha);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}