#include "Math/Math.h"

#include <climits>

Math::Range::Range() :
    min(INT_MAX), max(INT_MIN)
{
}

bool Math::Range::IsInRange(int value) const
{
    return value >= min && value <= max;
}

void Math::Range::AddValue(int value)
{
    if (this->IsEmpty()) {
        min = value;
        max = value;
    } else {
        if (value < min) {
            min = value;
        }
        if (value > max) {
            max = value;
        }
    }
}
bool Math::Range::IsEmpty() const
{
    return min == INT_MAX && max == INT_MIN;
}
void Math::Range::Reset()
{
    min = INT_MAX;
    max = INT_MIN;
}