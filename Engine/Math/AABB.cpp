#include "Math/AABB.h"
#include "AABB.h"

Vec2f AABB::GetCenter() const
{
    return Vec2f(
        corner.x + size.x / 2,
        corner.y + size.y / 2
    );
}

void AABB::SetCenter(Vec2f& center)
{
    corner = Vec2f(
        center.x - size.x / 2,
        center.y - size.y / 2
    );
}

bool AABB::Contains(const Vec2f &point) const
{
    return (point.x >= corner.x && point.x <= corner.x + size.x &&
            point.y >= corner.y && point.y <= corner.y + size.y);
}

bool AABB::Contains(const AABB &other) const
{
    return (corner.x <= other.corner.x &&
            corner.x + size.x >= other.corner.x + other.size.x &&
            corner.y <= other.corner.y &&
            corner.y + size.y >= other.corner.y + other.size.y);
}

bool AABB::Overlaps(const AABB &other) const
{
    return (corner.x <= other.corner.x + other.size.x &&
            corner.x + size.x >= other.corner.x &&
            corner.y <= other.corner.y + other.size.y &&
            corner.y + size.y >= other.corner.y);
}

AABB AABB::Expand(float amount) const
{
    return AABB(
        corner.x - amount,
        corner.y - amount,
        size.x + amount * 2,
        size.y + amount * 2
    );
}

std::ostream &operator<<(std::ostream &os, const AABB &aabb)
{
    os << "AABB(" << aabb.corner << ", " << aabb.size << ")";
    return os;
}