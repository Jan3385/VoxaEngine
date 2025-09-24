#pragma once

#include <GameEngine.h>

#include "Rendering/ImGuiRenderer.h"

struct EditorStateStorage{
    Vec2i generateNewChunksSize = Vec2i(1, 1);

    void SetNewChunkSize(const Vec2i& size) { this->chunkSize = size; }
    Vec2i GetChunkSize() const { return this->chunkSize; }
private:
    Vec2i chunkSize = Vec2i(0, 0);
};

class Editor : public IGame {
public:
    ~Editor() override = default;
    static Editor instance;

    Vec2f cameraPosition = Vec2f(0, 0);
    EditorStateStorage stateStorage = {};
    ImGuiRenderer imguiRenderer;
private:
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
    void OnSceneChange(ChunkMatrix* oldMatrix, ChunkMatrix* newMatrix) override;
};