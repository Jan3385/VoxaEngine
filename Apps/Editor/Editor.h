#pragma once

#include <GameEngine.h>

#include "Rendering/ImGuiRenderer.h"

class Editor : public IGame {
public:
    ~Editor() override = default;
    Vec2f cameraPosition = Vec2f(0, 0);
private:
    ImGuiRenderer imguiRenderer;
    void OnInitialize() override;
    void OnShutdown() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float fixedDeltaTime) override;
    void VoxelUpdate(float deltaTime) override;
    void Render(glm::mat4 voxelProjection, glm::mat4 viewProjection) override;

    void RegisterVoxels() override;
    void RegisterVoxelObjects() override;

    void OnMouseScroll(int yOffset) override;
    void OnMouseButtonDown(int button) override;
    void OnMouseButtonUp(int button) override;
    void OnMouseMove(int x, int y) override;
    void OnKeyboardDown(int key) override;
    void OnKeyboardUp(int key) override;

    void OnWindowResize(int newX, int newY) override;
};