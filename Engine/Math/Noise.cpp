#include "Noise.h"

inline unsigned int Hash(int x, int y, int seed) {
    unsigned int h = x * 374761393u + y * 668265263u + seed * 951274213u; 
    h = (h ^ (h >> 13u)) * 1274126177u;
    return h ^ (h >> 16u);
}

inline float PseudoRandomFloat(int x, int y, int seed) {
    return (Hash(x, y, seed) & 0xFFFFFF) / float(0xFFFFFF);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float smoothstep(float t) {
    return t * t * (3 - 2 * t);
}

float Noise::ValueNoise(const Vec2f &pos, int seed)
{
    int x0 = static_cast<int>(floor(pos.x));
    int y0 = static_cast<int>(floor(pos.y));
    float fx = pos.x - x0;
    float fy = pos.y - y0;

    float v00 = PseudoRandomFloat(x0,     y0,     seed);
    float v10 = PseudoRandomFloat(x0 + 1, y0,     seed);
    float v01 = PseudoRandomFloat(x0,     y0 + 1, seed);
    float v11 = PseudoRandomFloat(x0 + 1, y0 + 1, seed);

    float u = smoothstep(fx);
    float v = smoothstep(fy);

    float ix0 = lerp(v00, v10, u);
    float ix1 = lerp(v01, v11, u);

    return lerp(ix0, ix1, v);
}