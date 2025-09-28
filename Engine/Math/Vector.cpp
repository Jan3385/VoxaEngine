#include "Math/Vector.h"
#include <stdexcept>
#include <cmath>
#include "Vector.h"

Vec2f::Vec2f()
{
}

Vec2f::Vec2f(float x, float y)
{
    this->x = x;
    this->y = y;
}

Vec2f::Vec2f(const Vec2i vec)
{
    this->x = vec.x;
    this->y = vec.y;
}

Vec2f::~Vec2f()
{
}

Vec2f Vec2f::operator+(const Vec2f &other) const
{
    return Vec2f(this->x + other.x, this->y + other.y);
}

Vec2f Vec2f::operator-(const Vec2f &other) const
{
    return Vec2f(this->x - other.x, this->y - other.y);
}

Vec2f Vec2f::operator+=(const Vec2f &other)
{
    this->x = this->x + other.x;
    this->y = this->y + other.y;
    return *this;
}

Vec2f Vec2f::operator*(const int &other) const
{
    return Vec2f(
        this->x * other,
        this->y * other
    );
}

Vec2f Vec2f::operator*(const float &other) const
{
    return Vec2f(
        this->x * other,
        this->y * other
    );
}

Vec2f Vec2f::operator/(const float &other) const
{
    if (other == 0.0f) {
        throw std::runtime_error("Division by zero in Vec2f::operator/");
    }
    return Vec2f(
        this->x / other,
        this->y / other
    );
}

bool Vec2f::operator==(const Vec2f &other) const
{
    return this->x == other.x && this->y == other.y;
}

bool Vec2f::operator!=(const Vec2f &other) const
{
    return this->x != other.x || this->y != other.y;
}

Vec2i::Vec2i()
{
}

Vec2i::Vec2i(int x, int y)
{
    this->x = x;
    this->y = y;
}

Vec2i::Vec2i(const Vec2f vec)
{
    this->x = vec.x;
    this->y = vec.y;
}

Vec2i::~Vec2i()
{
}
void Vec2i::RotateLeft()
{
    int temp = this->x;
    this->x = this->y;
    this->y = -temp;
}

void Vec2i::RotateRight()
{
    int temp = this->x;
    this->x = -this->y;
    this->y = temp;
}

void Vec2i::Rotate180()
{
    this->x = -this->x;
    this->y = -this->y;
}

float Vec2i::Length() const
{
    return std::sqrt(this->x * this->x + this->y * this->y);
}

float Vec2i::LengthSquared() const
{
    return this->x * this->x + this->y * this->y;
}

float Vec2f::Length() const
{
    return std::sqrt(this->x * this->x + this->y * this->y);
}

float Vec2f::LengthSquared() const
{
    return this->x * this->x + this->y * this->y;
}

Vec2i Vec2f::Round() const
{
    return Vec2i(std::round(this->x), std::round(this->y));
}

Vec2i Vec2f::Floor() const
{
    return Vec2i(std::floor(this->x), std::floor(this->y));
}

Vec2i Vec2f::Ceil() const
{
    return Vec2i(std::ceil(this->x), std::ceil(this->y));
}

Vec2i Vec2i::operator+(const Vec2i &other) const
{
    return Vec2i(this->x + other.x, this->y + other.y);
}

Vec2i Vec2i::operator-(const Vec2i &other) const
{
    return Vec2i(this->x - other.x, this->y - other.y);
}

Vec2i Vec2i::operator+=(const Vec2i &other)
{
    this->x = this->x + other.x;
    this->y = this->y + other.y;
    return *this;
}

Vec2i Vec2i::operator-=(const Vec2i &other)
{
    this->x = this->x - other.x;
    this->y = this->y - other.y;
    return *this;
}

Vec2i Vec2i::operator*(const Vec2i &other) const
{
    return Vec2i(
        this->x * other.x,
        this->y * other.y
    );
}
Vec2i Vec2i::operator*(const int &other) const
{
    return Vec2i(
        this->x * other,
        this->y * other
    );
}

bool Vec2i::operator!=(const Vec2i &other) const
{
    return this->x != other.x || this->y != other.y;
}

bool Vec2i::operator==(const Vec2i &other) const
{
    return this->x == other.x && this->y == other.y;
}

std::ostream &operator<<(std::ostream &os, const Vec2f &vec)
{
    os << "Vec2f(" << vec.x << ", " << vec.y << ")";
    return os;
}

std::ostream &operator<<(std::ostream &os, const Vec2i &vec)
{
    os << "Vec2i(" << vec.x << ", " << vec.y << ")";
    return os;
}
