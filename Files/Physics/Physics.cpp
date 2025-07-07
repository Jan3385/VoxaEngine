#include "Physics/Physics.h"

#include <iostream>
#include <queue>
#include "Physics.h"

GamePhysics::GamePhysics()
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2(0.0f, -9.81f);
    
    worldId = b2CreateWorld(&worldDef);
}

GamePhysics::~GamePhysics()
{
    b2DestroyWorld(worldId);
}

/// @brief                  Basic flood fill algorithm
/// @param values           2D array. true if part of the fill area, false otherwise
/// @param labels           2D array to store labels for each cell
/// @param start            starting point for the flood fill
/// @param currentLabel     current label to assign to the filled area
/// @return                 true if the flood fill was successful, false otherwise
bool GamePhysics::FloodFillChunk(
    const bool values[Volume::Chunk::CHUNK_SIZE][Volume::Chunk::CHUNK_SIZE], 
    int labels[Volume::Chunk::CHUNK_SIZE][Volume::Chunk::CHUNK_SIZE], 
    Vec2i start, int currentLabel)
{
    if(values[start.x][start.y] == false){
        //labels[start.x] = -1;
        return false;       // not a valid starting point
    }

    std::queue<Vec2i> queue;
    queue.push(start);
    
    return true;
}

void GamePhysics::Step(float deltaTime)
{
    b2World_Step(worldId, deltaTime, this->SIMULATION_STEP_COUNT);
}
