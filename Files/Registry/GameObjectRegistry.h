#pragma once

#include "Math/Vector.h"
#include "GameObject/GameObject.h"
#include <SDL.h>
#include "World/ChunkMatrix.h"

namespace Registry
{
    void CreateGameObject(ChunkMatrix *matrix, SDL_Texture *texture, Vec2f position);
}
