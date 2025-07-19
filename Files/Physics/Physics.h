#pragma once

#include <box2d/box2d.h>

#include "Math/Vector.h"
#include "World/Chunk.h"
#include "GameObject/PhysicsObject.h"

struct Triangle{
    b2Vec2 a;
    b2Vec2 b;
    b2Vec2 c;

    Triangle(const b2Vec2& a, const b2Vec2& b, const b2Vec2& c)
        : a(a), b(b), c(c) {};
};

class GamePhysics{
private:
    static constexpr float PHYS_OBJECT_GRAVITY = 9.81f;
    static constexpr float SIMULATION_SPEED = 5.0f;
    static constexpr int SIMULATION_STEP_COUNT = 4;
    b2WorldId worldId;
public:
    GamePhysics();
    ~GamePhysics();
    void Step(float deltaTime);

    std::vector<PhysicsObject*> physicsObjects;
    b2WorldId GetWorldId() const { return worldId; }

    void Generate2DCollidersForChunk(
        Volume::Chunk* chunk
    );
    void Generate2DCollidersForVoxelObject(
        PhysicsObject* object
    );
};