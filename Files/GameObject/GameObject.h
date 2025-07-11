#pragma once

#include <SDL.h>
#include <GL/glew.h>

#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Chunk.h"

class GameObject{
public:
    GameObject() = default;
    GameObject(Vec2f position, std::string texturePath);
    virtual ~GameObject();

    void SetEnabled(bool enabled) { this->enabled = enabled; }

    // Disable copy and move semantics
    GameObject(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    virtual void Update(ChunkMatrix& chunkMatrix, float deltaTime) {};

    virtual bool ShouldRender() const { return true; };

    Vec2f GetPosition() const {
        return position;
    }
    Vec2i GetSize() const {
        return Vec2i(width, height);
    }
    GLuint GetTexture() const {
        return texture;
    }
protected:
    bool enabled = false;
    Vec2f position;
    float rotation = 0.0f; // in radians
    int width = 0;
    int height = 0;
    GLuint texture = 0;
};