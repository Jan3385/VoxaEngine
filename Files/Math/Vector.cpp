#include "Vector.h"

const float Vec2::x()
{
    return m_x;
}

const float Vec2::y()
{
    return m_y;
}

void Vec2::x(float x)
{
    m_x = x;
}

void Vec2::y(float y)
{
    m_y = y;
}

Vec2::Vec2()
{
}

Vec2::Vec2(float x, float y)
{
    this->x(x);
    this->y(y);
}

Vec2::~Vec2()
{
}
