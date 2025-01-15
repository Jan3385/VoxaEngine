#include "AABB.h"

Vec2f AABB::GetCenter() const
{
    return Vec2f(
        corner.getX() + size.getX() / 2,
        corner.getY() + size.getY() / 2
    );
}

void AABB::SetCenter(Vec2f& center)
{
    corner = Vec2f(
        center.getX() - size.getX() / 2,
        center.getY() - size.getY() / 2
    );
}

bool AABB::Contains(const Vec2f &point) const
{
    return (point.getX() >= corner.getX() && point.getX() <= corner.getX() + size.getX() &&
            point.getY() >= corner.getY() && point.getY() <= corner.getY() + size.getY());
}

bool AABB::Overlaps(const AABB &other) const
{
    return (corner.getX() <= other.corner.getX() + other.size.getX() &&
            corner.getX() + size.getX() >= other.corner.getX() &&
            corner.getY() <= other.corner.getY() + other.size.getY() &&
            corner.getY() + size.getY() >= other.corner.getY());
}
