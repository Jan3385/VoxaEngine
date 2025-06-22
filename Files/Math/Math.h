#pragma once

#include "Math/AABB.h"
#include "Math/Color.h"
#include "Math/Vector.h"
#include "Math/Temperature.h"

#include <cstdint>
#include <stdexcept>

namespace Math{
    class Range{
    private:
        int min;
        int max;
    public:
        Range(int min, int max) : min(min), max(max) {
            if(min > max) {
                throw std::invalid_argument("Min cannot be greater than max");
            }
        }
        Range();

        bool IsInRange(int value) const;

        void AddValue(int value);

        bool IsEmpty() const;
        void Reset();

        int Start() const { return min; }
        int End() const { return max; }
    };
}