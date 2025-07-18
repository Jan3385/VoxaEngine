#pragma once

#include <vector>
#include <cmath>

#include "Math/Vector.h"

/// @brief Fast rotation of a 2D vector grid.
/// @tparam T Type of the elements in the grid.
/// @param input input grid of vectors to rotate
/// @param output output grid of rotated vectors
/// @param angle angle in radians to rotate the vectors
template<typename T>
void FastRotate2DVector(const std::vector<std::vector<T*>>& input, std::vector<std::vector<T*>>& output, float angle)
{
    double cosAngle = cos(angle);
    double sinAngle = sin(angle);

    output.clear();
    Vec2i outputSize(
        ceil(abs(input.size() * cosAngle) + abs(input[0].size() * sinAngle)),
        ceil(abs(input.size() * sinAngle) + abs(input[0].size() * cosAngle))
    );

    output.resize(outputSize.x, std::vector<T*>(outputSize.y, nullptr));


    Vec2f center(
        static_cast<float>((input.size() - 1) / 2.0f),
        static_cast<float>((input[0].size() - 1) / 2.0f)
    );

    Vec2f outputCenter(
        static_cast<float>((output.size() - 1) / 2.0f),
        static_cast<float>((output[0].size() - 1) / 2.0f)
    );

    for (int y = 0; y < static_cast<int>(output[0].size()); ++y) {
        for (int x = 0; x < static_cast<int>(output.size()); ++x) {
            Vec2f dVec = Vec2i(x, y) - outputCenter;

            Vec2f srcVec(
                cosAngle * dVec.x + sinAngle * dVec.y + center.x,
                -sinAngle * dVec.x + cosAngle * dVec.y + center.y
            );

            Vec2i neighbourVec(
                static_cast<int>(round(srcVec.x)),
                static_cast<int>(round(srcVec.y))
            );

            if (neighbourVec.x >= 0 && neighbourVec.x < static_cast<int>(input.size()) &&
                neighbourVec.y >= 0 && neighbourVec.y < static_cast<int>(input[0].size())) {
                output[x][y] = input[neighbourVec.x][neighbourVec.y];
            } else {
                output[x][y] = nullptr; // Out of bounds, set to nullptr
            }
        }
    }
}