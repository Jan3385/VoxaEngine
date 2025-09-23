#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <array>
#include <ostream>

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

    friend std::ostream& operator<<(std::ostream& os, const RGB& color);
};

class RGBA : public RGB{
public:
    uint8_t a;

    RGBA operator*(const RGBA& other) const;

    RGBA();
    RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    RGBA(uint32_t color); // ARGB format
    RGBA(std::array<float, 4> color); // RGBA format
    glm::vec4 getGLMVec4() const;
    std::array<float, 4> toFloatArray() const;
    ~RGBA();

    friend std::ostream& operator<<(std::ostream& os, const RGBA& color);
};


