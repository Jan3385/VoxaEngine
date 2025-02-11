#include "Vector.h"

float Vec2f::getX() const
{
    return m_x;
}

float Vec2f::getY() const
{
    return m_y;
}

void Vec2f::x(float x)
{
    m_x = x;
}

void Vec2f::y(float y)
{
    m_y = y;
}

Vec2f::Vec2f()
{
}

Vec2f::Vec2f(float x, float y)
{
    this->x(x);
    this->y(y);
}

Vec2f::Vec2f(const Vec2i vec)
{
    this->x(vec.getX());
    this->y(vec.getY());
}

Vec2f::~Vec2f()
{
}

Vec2f Vec2f::operator+(const Vec2f &other) const
{
    return Vec2f(this->getX() + other.getX(), this->getY() + other.getY());
}

Vec2f Vec2f::operator-(const Vec2f &other) const
{
    return Vec2f(this->getX() - other.getX(), this->getY() - other.getY());
}

Vec2f Vec2f::operator+=(const Vec2f &other)
{
    this->x(this->getX() + other.getX());
    this->y(this->getY() + other.getY());
    return *this;
}

Vec2f Vec2f::operator*(const int &other) const
{
    return Vec2f(
        this->getX() * other,
        this->getY() * other
    );
}

int Vec2i::getX() const
{
    return m_x;
}

int Vec2i::getY() const
{
    return m_y;
}

void Vec2i::x(int x)
{
    m_x = x;
}

void Vec2i::y(int y)
{
    m_y = y;
}

Vec2i::Vec2i()
{
}

Vec2i::Vec2i(int x, int y)
{
    this->x(x);
    this->y(y);
}

Vec2i::Vec2i(const Vec2f vec)
{
    this->x(vec.getX());
    this->y(vec.getY());
}

Vec2i::~Vec2i()
{
}

Vec2i Vec2i::operator+(const Vec2i &other) const
{
    return Vec2i(this->getX() + other.getX(), this->getY() + other.getY());
}

Vec2i Vec2i::operator-(const Vec2i &other) const
{
    return Vec2i(this->getX() - other.getX(), this->getY() - other.getY());
}

Vec2i Vec2i::operator+=(const Vec2i &other)
{
    this->x(this->getX() + other.getX());
    this->y(this->getY() + other.getY());
    return *this;
}

Vec2i Vec2i::operator-=(const Vec2i &other)
{
    this->x(this->getX() - other.getX());
    this->y(this->getY() - other.getY());
    return *this;
}

Vec2i Vec2i::operator*(const int &other) const
{
    return Vec2i(
        this->getX() * other,
        this->getY() * other
    );
}

bool Vec2i::operator!=(const Vec2i &other) const
{
    return this->getX() != other.getX() || this->getY() != other.getY();
}

bool Vec2i::operator==(const Vec2i &other) const
{
    return this->getX() == other.getX() && this->getY() == other.getY();
}
