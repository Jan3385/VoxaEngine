#pragma once

#include <GameEngine.h>
#include "GameObject/Player/Player.h"

class Game : public IGame {
    static Player *player;

    void OnInitialize() override;
    void OnShutdown() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float fixedDeltaTime) override;
    void PhysicsUpdate(float deltaTime) override;
    void Render() override;

    void OnMouseButtonDown(int button) override;
    void OnMouseButtonUp(int button) override;
    void OnMouseMove(int x, int y) override;
    void OnKeyboardDown(int key) override;
    void OnKeyboardUp(int key) override;

    void OnWindowResize(int newX, int newY) override;
};