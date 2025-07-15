#include "Math/Color.h"
#include "Color.h"

RGB::RGB() : r(0), g(0), b(0) { }
RGB::RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

RGBA::RGBA() : RGB(), a(0) { }

RGBA::RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : RGB(r, g, b), a(a) {};

RGB::~RGB() {}
RGBA RGB::toRGBA()
{
    return RGBA(r, g, b, 255);
}
// constructor from ARGB format
RGBA::RGBA(uint32_t color)
{
    this->a = (color >> 24) & 0xFF;
    this->r = (color >> 16) & 0xFF;
    this->g = (color >> 8) & 0xFF; 
    this->b = color & 0xFF;        
}
glm::vec4 RGBA::getGLMVec4() const
{
    constexpr float scale = 1.0f / 255.0f;
    return glm::vec4(
        static_cast<float>(this->r) * scale,
        static_cast<float>(this->g) * scale,
        static_cast<float>(this->b) * scale,
        static_cast<float>(this->a) * scale
    );
}

RGBA::~RGBA() {}