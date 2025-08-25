#include "Math/Color.h"
#include "Color.h"

RGB RGB::operator*(const RGB &other) const
{
    return RGB(
        static_cast<uint8_t>(this->r * other.r / 255),
        static_cast<uint8_t>(this->g * other.g / 255),
        static_cast<uint8_t>(this->b * other.b / 255)
    );
}

RGB::RGB() : r(0), g(0), b(0) {}
RGB::RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

/// @brief Mix two RGB colors.
/// @param other The other color to mix with.
/// @param factor The factor to mix the colors <0.0 - 1.0>.
/// @return The mixed color.
RGB RGB::Mix(const RGB &other, float factor) const
{
    return RGB(
        static_cast<uint8_t>(this->r * (1 - factor) + other.r * factor),
        static_cast<uint8_t>(this->g * (1 - factor) + other.g * factor),
        static_cast<uint8_t>(this->b * (1 - factor) + other.b * factor)
    );
}

RGBA RGBA::operator*(const RGBA &other) const
{
    return RGBA(
        static_cast<uint8_t>(this->r * other.r / 255),
        static_cast<uint8_t>(this->g * other.g / 255),
        static_cast<uint8_t>(this->b * other.b / 255),
        static_cast<uint8_t>(this->a * other.a / 255)
    );
}

RGBA::RGBA() : RGB(), a(0) {}

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