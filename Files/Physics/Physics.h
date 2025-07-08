#pragma once

#include <box2d/box2d.h>

#include <Math/Vector.h>
#include <World/Chunk.h>

class GamePhysics{
private:
    static constexpr int GRID_PADDING_FILL = 1;
    static constexpr int SIMULATION_STEP_COUNT = 4;
    b2WorldId worldId;

    static void FloodFillChunk(
        const bool values[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL], 
        int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
        Vec2i start, unsigned short int currentLabel
    );

    static std::vector<b2Vec2> MarchingSquaresEdgeTrace(
        const int labels[Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL][Volume::Chunk::CHUNK_SIZE+GRID_PADDING_FILL],
        const int currentLabel
    );

public:
    GamePhysics();
    ~GamePhysics();
    void Step(float deltaTime);

    void Generate2DCollidersForChunk(
        Volume::Chunk* chunk, glm::mat4 voxelProj
    );
};