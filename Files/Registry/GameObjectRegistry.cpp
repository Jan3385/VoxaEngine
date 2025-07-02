#include "Registry/GameObjectRegistry.h"
#include "GameObjectRegistry.h"

void Registry::CreateGameObject(ChunkMatrix *matrix, Vec2f position, std::string texturePath)
{
    GameObject *gameObject = new GameObject(position, texturePath);
    matrix->gameObjects.push_back(gameObject);
}