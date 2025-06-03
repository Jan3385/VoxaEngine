#pragma once

#include <box2d/box2d.h>

class GamePhysics{
private:
    static constexpr int SIMULATION_STEP_COUNT = 4;
    b2WorldId worldId;
public:
    GamePhysics();
    ~GamePhysics();
    void Step(float deltaTime);
};