layout(location = 0) in vec2 quadVertex;         // Quad vertex (0-1)
layout(location = 1) in ivec2 instancePos; // Voxel position
layout(location = 2) in vec4 instanceColor;// Voxel color
layout(location = 3) in float aHeat;       // Heat value

uniform mat4 projection;                    // Projection matrix

uniform bool showHeatAroundCursor;          // Show heat around cursor
uniform vec2 cursorPosition;               // Cursor position in world coordinates (only used if showHeatAroundCursor is true)

out vec4 vertexColor;                       // output color to fragment shader

// Returns a color based on the heat value in degrees Celsius (approximate blackbody/thermal emission)
vec3 HeatToColor(float tempC) {
    tempC = clamp(tempC, 0.0, 2000.0);
    vec3 color;
    if (tempC < 525.0) {
        // 0°C - 525°C: dark to red
        float t = tempC / 525.0;
        color = mix(vec3(0.1, 0.0, 0.0), vec3(1.0, 0.0, 0.0), t);
    } else if (tempC < 800.0) {
        // 525°C - 800°C: red to orange
        float t = (tempC - 525.0) / (800.0 - 525.0);
        color = mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 0.5, 0.0), t);
    } else if (tempC < 1200.0) {
        // 800°C - 1200°C: orange to yellow
        float t = (tempC - 800.0) / (1200.0 - 800.0);
        color = mix(vec3(1.0, 0.5, 0.0), vec3(1.0, 1.0, 0.0), t);
    } else if (tempC < 1600.0) {
        // 1200°C - 1600°C: yellow to white
        float t = (tempC - 1200.0) / (1600.0 - 1200.0);
        color = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0), t);
    } else {
        // 1600°C - 2000°C: white (saturated)
        color = vec3(1.0, 1.0, 1.0);
    }
    return color;
}
vec3 HeatToIndicatorColor(float tempC) {
    tempC = clamp(tempC, 0.0, 2000.0);
    vec3 color;
    if (tempC < -10.0) {
        float t = (tempC + 10.0) / 10.0;
        color = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), t); // blue to cyan
    } else if (tempC < 10.0) {
        float t = tempC / 10.0;
        color = mix(vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), t); // cyan to green
    } else if (tempC < 100.0) {
        float t = tempC / 100.0;
        color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), t); // green to yellow
    } else if (tempC < 100.0) {
        float t = (tempC - 100.0) / (500.0 - 100.0);
        color = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.5, 0.0), t); // yellow to orange
    } else if (tempC < 500.0) {
        float t = (tempC - 100.0) / (500.0 - 100.0);
        color = mix(vec3(1.0, 0.5, 0.0), vec3(1.0, 0.0, 0.0), t); // orange to red
    } else {
        float t = (tempC - 500.0) / (2000.0 - 500.0);
        color = mix(vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), t); // red to white
    }
    return color;
}

const float maxAlpha = 0.1;
const float distanceThreshold = 50.0;

void main()
{
    gl_Position = projection * vec4(instancePos + quadVertex, 0.0, 1.0);

    // alpha: 0 at 100°C, 0.5 at 2000°C, clamp in between
    float alpha = clamp((aHeat - 100.0) / (2000.0 - 100.0) * maxAlpha, 0.0, maxAlpha);

    vertexColor = vec4(HeatToColor(aHeat), alpha);

    if(showHeatAroundCursor) {
        float distance = length(instancePos + quadVertex - cursorPosition);
        vec3 heatColor = HeatToIndicatorColor(aHeat);
        float alpha = clamp(1.0 - distance / distanceThreshold, 0.0, 0.4);
        vertexColor = mix(vertexColor, vec4(heatColor, 1), alpha);
    }
}
