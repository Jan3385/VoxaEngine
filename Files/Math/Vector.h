#pragma once

class Vec2i;

class Vec2f
{
    float m_x;
    float m_y;
public:
    const float getX() const;
    const float getY() const;
    void x(float x);
    void y(float y);
    Vec2f();
    Vec2f(float x, float y);
    Vec2f(const Vec2i vec);
    Vec2f(const Vec2f& vec);
    ~Vec2f();
    Vec2f operator+(const Vec2f& other) const;
    Vec2f operator+=(const Vec2f& other);
    Vec2f operator*(const int& other);
};
class Vec2i
{
private:
    int m_x;
    int m_y;
public:
    const int getX() const;
    const int getY() const;
    void x(int x);
    void y(int y);
    Vec2i();
    Vec2i(int x, int y);
    Vec2i(const Vec2f vec);
    ~Vec2i();
    Vec2i operator+(const Vec2i& other) const;
    Vec2i operator+=(const Vec2i& other);
    Vec2i operator-=(const Vec2i& other);
    bool operator!=(const Vec2i& other) const;
    bool operator==(const Vec2i& other) const;
};
