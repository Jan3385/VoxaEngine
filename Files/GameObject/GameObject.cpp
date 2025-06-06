#include "GameObject.h"

#include <iostream>

GameObject::GameObject(SDL_Texture *texture, Vec2f position)
{
    if(!texture) {
        std::cerr << "Error: GameObject texture is null!" << std::endl;
        return;
    }

    this->texture = texture;

    SDL_QueryTexture(this->texture, NULL, NULL, &this->width, &this->height);

    this->position = position;
}

GameObject::~GameObject()
{
    if (this->texture) {
        SDL_DestroyTexture(this->texture);
        this->texture = nullptr;
    }
}

void GameObject::Render(SDL_Renderer *renderer, const Vec2f &offset)
{
    if (!renderer || !this->texture) {
        std::cerr << "Error: Renderer or texture is null!" << std::endl;
        return;
    }

    SDL_Rect destRect = {
        static_cast<int>((this->position.getX() - offset.getX()) * Volume::Chunk::RENDER_VOXEL_SIZE),
        static_cast<int>((this->position.getY() - offset.getY()) * Volume::Chunk::RENDER_VOXEL_SIZE),
        this->width* Volume::Chunk::RENDER_VOXEL_SIZE,
        this->height * Volume::Chunk::RENDER_VOXEL_SIZE
    };

    SDL_RenderCopy(renderer, this->texture, NULL, &destRect);
}
