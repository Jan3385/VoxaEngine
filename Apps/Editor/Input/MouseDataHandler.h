#pragma once

#include <string>
#include <Math/Color.h>

namespace Input{
    struct MouseData{
        bool leftButtonDown = false;
        bool rightButtonDown = false;
        RGBA placeColor = RGBA(255, 255, 255, 255);
        int brushRadius = 0;
    };
}