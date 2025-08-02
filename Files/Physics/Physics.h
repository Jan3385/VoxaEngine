#pragma once

#include <box2d/box2d.h>

#include <list>

#include "Math/Vector.h"
#include "World/Chunk.h"
#include "GameObject/PhysicsObject.h"
#include "Physics/Triangle.h"

class GamePhysics{
private:
    static constexpr float PHYS_OBJECT_GRAVITY = 9.81f;
    static constexpr float SIMULATION_SPEED = 5.0f;
    static constexpr int SIMULATION_STEP_COUNT = 4;
    b2WorldId worldId;
public:
    GamePhysics();
    ~GamePhysics();
    void Step(float deltaTime, ChunkMatrix& chunkMatrix);

    std::list<PhysicsObject*> physicsObjects;
    b2WorldId GetWorldId() const { return worldId; }

    void Generate2DCollidersForChunk(
        Volume::Chunk* chunk
    );
    void Generate2DCollidersForVoxelObject(
        PhysicsObject* object,
        ChunkMatrix* chunkMatrix
    );
};