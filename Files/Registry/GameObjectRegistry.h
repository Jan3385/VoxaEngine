#pragma once

#include <SDL.h>

#include "Math/Vector.h"
#include "GameObject/GameObject.h"
#include "GameObject/PhysicsObject.h"
#include "World/ChunkMatrix.h"
#include "Physics/Physics.h"

namespace Registry
{
    void CreateGameObject(ChunkMatrix *matrix, Vec2f position, std::string texturePath);
    void CreatePhysicsObject(ChunkMatrix *matrix, GamePhysics *gamePhysics, Vec2f position, std::string texturePath);
}
