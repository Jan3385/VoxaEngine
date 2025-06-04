#include "Physics/Physics.h"

#include <iostream>

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

void GamePhysics::Step(float deltaTime)
{
    //b2World_Step(worldId, deltaTime, this->SIMULATION_STEP_COUNT);
}
