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
    RGB();
    RGB(uint8_t r, uint8_t g, uint8_t b);
    ~RGB();
    RGBA toRGBA();
};

class RGBA : public RGB{
public:
    uint8_t a;
    RGBA();
    RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    glm::vec4 getGLMVec4() const;
    ~RGBA();
};


