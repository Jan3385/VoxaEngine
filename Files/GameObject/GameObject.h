#pragma once

#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Chunk.h"

class GameObject{
public:
    GameObject() = default;
    GameObject(Vec2f position);
    virtual ~GameObject();

    // Disable copy and move semantics
    GameObject(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    virtual void Update(ChunkMatrix& chunkMatrix, float deltaTime) {};
    virtual void Render(SDL_Renderer* renderer, const Vec2f &offset);
protected:
    Vec2f position;
    int width = 0;
    int height = 0;
    SDL_Texture* texture = nullptr;
};