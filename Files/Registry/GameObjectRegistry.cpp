#include "Registry/GameObjectRegistry.h"
#include "GameObjectRegistry.h"

void Registry::CreateGameObject(ChunkMatrix *matrix, SDL_Texture *texture, Vec2f position)
{
    GameObject *gameObject = new GameObject(texture, position);
    matrix->gameObjects.push_back(gameObject);
}