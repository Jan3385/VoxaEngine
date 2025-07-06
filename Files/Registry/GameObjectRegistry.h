#pragma once

#include "Math/Vector.h"
#include "GameObject/GameObject.h"
#include <SDL.h>
#include "World/ChunkMatrix.h"

namespace Registry
{
    void CreateGameObject(ChunkMatrix *matrix, Vec2f position, std::string texturePath);
}
