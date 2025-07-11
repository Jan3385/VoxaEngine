#include "GameObject.h"

#include "Rendering/SpriteRenderer.h"
#include "GameEngine.h"

#include <iostream>
#include <algorithm>

GameObject::GameObject(Vec2f position, std::string texturePath)
{
    this->position = position;

    Vec2i size;
    this->texture = SpriteRenderer::LoadTextureFromFile(texturePath, &size);
    this->width = size.x;
    this->height = size.y;

    this->enabled = true;
}

GameObject::~GameObject()
{
    this->enabled = false;
    if (this->texture != 0) {
        glDeleteTextures(1, &this->texture);
        this->texture = 0;
    }

    std::vector<GameObject*>& vec = GameEngine::instance->chunkMatrix.gameObjects;
    vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
}
