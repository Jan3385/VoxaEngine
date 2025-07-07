#pragma once

#include <box2d/box2d.h>

#include <Math/Vector.h>
#include <World/Chunk.h>

class GamePhysics{
private:
    static constexpr int SIMULATION_STEP_COUNT = 4;
    b2WorldId worldId;

    static bool FloodFillChunk(
        const bool values[Volume::Chunk::CHUNK_SIZE][Volume::Chunk::CHUNK_SIZE], 
        int labels[Volume::Chunk::CHUNK_SIZE][Volume::Chunk::CHUNK_SIZE],
        Vec2i start, int currentLabel
    );

public:
    GamePhysics();
    ~GamePhysics();
    void Step(float deltaTime);
};