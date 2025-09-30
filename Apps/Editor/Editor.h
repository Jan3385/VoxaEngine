#pragma once

#include <GameEngine.h>

#include "Scene.h"
#include "Rendering/ImGuiRenderer.h"

struct EditorStateStorage{
    EditorScene::Type selectedSceneType = EditorScene::Type::ObjectEditor;

    Volume::State selectedVoxelState = Volume::State::Solid;
    bool placedSimVoxelsAreUnmovable = false;
    float simulationFPS = 30.0f;
    bool runSimulationAuto = false;

    Vec2i generateNewChunksSize = Vec2i(1, 1);
    bool loadColorFromBMP = true;
};

class Editor : public IGame {
public:
    ~Editor() override = default;
    static Editor instance;

    Vec2f cameraPosition = Vec2f(0, 0);
    EditorStateStorage stateStorage = {};
    ImGuiRenderer imguiRenderer;
    
    std::vector<EditorScene> scenes = {};
    size_t activeSceneIndex = 0;
    EditorScene* GetActiveScene();
    void SwitchToScene(size_t index);

    void SetSimulationActive(bool state);

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

    bool isInDefaultScene = true;
    void OnSceneChange(ChunkMatrix* oldMatrix, ChunkMatrix* newMatrix) override;
};