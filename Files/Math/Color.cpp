#include "Math/Color.h"

RGB::RGB() : r(0), g(0), b(0) { }
RGB::RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

RGBA::RGBA() : RGB(), a(0) { }

RGBA::RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : RGB(r, g, b), a(a) {};

RGB::~RGB() {}
RGBA RGB::toRGBA()
{
    return RGBA(r, g, b, 255);
}
RGBA::~RGBA() {}