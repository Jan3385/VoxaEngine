#pragma once

class Vec2i;

class Vec2f
{
public:
    float x;
    float y;
    Vec2f();
    Vec2f(float x, float y);
    Vec2f(const Vec2i vec);
    ~Vec2f();

    void RotateLeft();
    void RotateRight();
    void Rotate180();

    float Length() const;
    float LengthSquared() const;

    Vec2f operator+(const Vec2f& other) const;
    Vec2f operator-(const Vec2f& other) const;
    Vec2f operator+=(const Vec2f& other);
    Vec2f operator*(const int& other) const;
    Vec2f operator*(const float& other) const;
    Vec2f operator/(const float& other) const;
    bool operator==(const Vec2f& other) const;
    bool operator!=(const Vec2f& other) const;
};
class Vec2i
{
public:
    int x;
    int y;
    Vec2i();
    Vec2i(int x, int y);
    Vec2i(const Vec2f vec);
    ~Vec2i();

    void RotateLeft();
    void RotateRight();
    void Rotate180();

    float Length() const;
    float LengthSquared() const;

    Vec2i operator+(const Vec2i& other) const;
    Vec2i operator-(const Vec2i& other) const;
    Vec2i operator+=(const Vec2i& other);
    Vec2i operator-=(const Vec2i& other);
    Vec2i operator*(const Vec2i& other) const;
    Vec2i operator*(const int& other) const;
    bool operator!=(const Vec2i& other) const;
    bool operator==(const Vec2i& other) const;
};

namespace vector{
    const Vec2i AROUND8[8] = {
        Vec2i(1, 0),
        Vec2i(1, 1),
        Vec2i(0, 1),
        Vec2i(-1, 1),
        Vec2i(-1, 0),
        Vec2i(-1, -1),
        Vec2i(0, -1),
        Vec2i(1, -1)
    };
    const Vec2i AROUND4[4] = {
        Vec2i(1, 0),
        Vec2i(0, 1),
        Vec2i(-1, 0),
        Vec2i(0, -1)
    };
    const Vec2i AROUND4C[4] = {
        Vec2i(1, 0),
        Vec2i(0, 1),
        Vec2i(-1, 0),
        Vec2i(0, -1)
    };
    const Vec2i AROUND4PLUS1[5] = {
        Vec2i(1, 0),
        Vec2i(0, 1),
        Vec2i(-1, 0),
        Vec2i(0, -1),
        Vec2i(0, 0)
    };
    const Vec2i AROUND8PLUS1[9] = {
        Vec2i(1, 0),
        Vec2i(1, 1),
        Vec2i(0, 1),
        Vec2i(-1, 1),
        Vec2i(-1, 0),
        Vec2i(-1, -1),
        Vec2i(0, -1),
        Vec2i(1, -1),
        Vec2i(0, 0)
    };
    // (0, -1)
    const Vec2i UP = Vec2i(0, -1);
    // (0, 1)
    const Vec2i DOWN = Vec2i(0, 1);
    // (-1, 0)
    const Vec2i LEFT = Vec2i(-1, 0);
    // (1, 0)
    const Vec2i RIGHT = Vec2i(1, 0);
    // (0, 0)
    const Vec2i ZERO = Vec2i(0, 0);
};