#include "GameObject.h"

#include "Rendering/SpriteRenderer.h"

#include <iostream>

GameObject::GameObject(Vec2f position, std::string texturePath)
{
    this->position = position;

    Vec2i size;
    this->texture = SpriteRenderer::LoadTextureFromFile(texturePath, &size);
    this->width = size.x;
    this->height = size.y;
}

GameObject::~GameObject()
{
    if (this->texture != 0) {
        glDeleteTextures(1, &this->texture);
        this->texture = 0;
    }
}
