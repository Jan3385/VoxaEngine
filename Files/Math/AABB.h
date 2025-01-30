#pragma once

#include "Vector.h"

class AABB
{
private:
    
public:
    Vec2f corner; //Top left corner
    Vec2f size;   //Width and height
    AABB(Vec2f corner, Vec2f size) : corner(corner), size(size) {};
    AABB(float x, float y, float width, float height) : corner(Vec2f(x, y)), size(Vec2f(width, height)) {};
    AABB() : corner(Vec2f(0, 0)), size(Vec2f(0, 0)) {};
    ~AABB() {};

    Vec2f GetCenter() const;
    void SetCenter(Vec2f& center);
    bool Contains(const Vec2f& point) const;
    bool Overlaps(const AABB& other) const;
    AABB Expand(float amount) const;
};