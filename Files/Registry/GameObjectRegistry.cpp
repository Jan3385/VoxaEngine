#include "Registry/GameObjectRegistry.h"
#include "GameObjectRegistry.h"
#include "Physics/Physics.h"

void Registry::CreateGameObject(ChunkMatrix *matrix, Vec2f position, std::string texturePath)
{
    GameObject *gameObject = new GameObject(position, texturePath);
    matrix->gameObjects.push_back(gameObject);
}

void Registry::CreatePhysicsObject(ChunkMatrix *matrix, GamePhysics *gamePhysics, Vec2f position, std::string texturePath)
{
    PhysicsObject *physicsObject = new PhysicsObject(position, texturePath);
    matrix->gameObjects.push_back(physicsObject);

    gamePhysics->physicsObjects.push_back(physicsObject);
}
