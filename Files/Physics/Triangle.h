#pragma once

#include <box2d/box2d.h>

struct Triangle{
    b2Vec2 a;
    b2Vec2 b;
    b2Vec2 c;

    Triangle(const b2Vec2& a, const b2Vec2& b, const b2Vec2& c)
        : a(a), b(b), c(c) {};
};