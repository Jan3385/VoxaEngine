#pragma once

#include <string>

namespace Input{
    struct MouseData{
        bool placeUnmovableSolidVoxels = false;
        int placementRadius = 5;
        int placeVoxelAmount = 20;
        std::string placeVoxelType = "Sand";
        float placeVoxelTemperature = 21.0f;
    };
}