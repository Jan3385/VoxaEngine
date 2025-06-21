#include "Registry/GameObjectRegistry.h"
#include "GameObjectRegistry.h"

void Registry::CreateGameObject(ChunkMatrix *matrix, Vec2f position)
{
    GameObject *gameObject = new GameObject(position);
    matrix->gameObjects.push_back(gameObject);
}