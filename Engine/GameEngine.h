#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <list>

#include <GL/glew.h>

#include <SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <box2d/box2d.h>
#include <box2d/types.h>

#include "Rendering/Renderer.h"
#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Chunk.h"
#include "World/ChunkMatrix.h"
#include "Physics/Physics.h"

#define AVG_FPS_SIZE_COUNT 25

struct IGame{
public:
    virtual ~IGame() = default;

    virtual void OnInitialize() = 0;
    virtual void OnShutdown() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void FixedUpdate(float fixedDeltaTime) = 0;
    virtual void PhysicsUpdate(float deltaTime) = 0;
    virtual void Render(glm::mat4 voxelProjection, glm::mat4 viewProjection) = 0;

    virtual void RegisterVoxels() = 0;
    virtual void RegisterVoxelObjects() = 0;

    virtual void OnMouseScroll(int yOffset) = 0;
    virtual void OnMouseButtonDown(int button) = 0;
    virtual void OnMouseButtonUp(int button) = 0;
    virtual void OnMouseMove(int x, int y) = 0;
    virtual void OnKeyboardDown(int key) = 0;
    virtual void OnKeyboardUp(int key) = 0;

    virtual void OnWindowResize(int newX, int newY) = 0;
};

struct EngineConfig{
    RGBA backgroundColor;
    bool vsync;
};

class GameEngine
{
private:
    EngineConfig config;
    IGame *currentGame = nullptr;

    SDL_GLContext glContext;
    SDL_Event windowEvent;
    Uint64 LastFrameEndTime = SDL_GetPerformanceCounter();

    std::thread simulationThread;

    VoxelObject *player = nullptr;

    float fixedUpdateTimer = 0;
    std::atomic<float> simulationUpdateTimer = 0;

    //Deletes old chunks and updates steps for voxel celluar automata simulation
    void UpdateGridVoxel(int pass);

    //Fixed update, Handles heat and pressure simulation
    void FixedUpdate(IGame& game);

    //Simulation thread, handles voxel simulation
    void SimulationThread(IGame& game);

    void EndFrame();
    void Update(IGame& game);
    void PollEvents();
    void Render();

    void StartSimulationThread(IGame& game);

    void Initialize(const EngineConfig& config);
public:
    static GameRenderer* renderer;
    static GamePhysics* physics;
    static GameEngine* instance;

    static constexpr int MAX_FRAME_RATE = 60;
    float fixedDeltaTime = 1/30.0;
    float simulationFixedDeltaTime = 1/30.0;

    static bool MovementKeysHeld[4]; //W, S, A, D

    bool running = false;
    float deltaTime = 1/60.0;    // time between frames in seconds

    float FPS = 60;
    float avgFPS = 60;
    std::list<float> FPSHistory = {};

    Vec2i WindowSize = {800, 600};    

    bool runHeatSimulation = true;
    bool runPressureSimulation = true;
    bool runChemicalReactions = true;

    // Mouse position in screen coordinates
    Vec2f mousePos;

    ChunkMatrix chunkMatrix;

    GameEngine();

    void Run(IGame& game, const EngineConfig& config);

    void SetPlayer(VoxelObject *player);
    VoxelObject *GetPlayer() const { return player; };
    ~GameEngine();

    Volume::Chunk* LoadChunkInView(Vec2i pos);

    // disallow copying
    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
};