#pragma once
#include <cstdint>
#include <glm/glm.hpp>

class RGBA;

class RGB
{
public:
    uint8_t r;
    uint8_t g;
    uint8_t b;

    RGB operator*(const RGB& other) const;

    RGB();
    RGB(uint8_t r, uint8_t g, uint8_t b);
    RGB Mix(const RGB& other, float factor) const;
    ~RGB();
    RGBA toRGBA();
};

class RGBA : public RGB{
public:
    uint8_t a;

    RGBA operator*(const RGBA& other) const;

    RGBA();
    RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    RGBA(uint32_t color); // ARGB format
    glm::vec4 getGLMVec4() const;
    ~RGBA();
};


