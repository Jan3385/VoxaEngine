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

    /// @brief Runs once at the start
    virtual void OnInitialize() = 0;
    /// @brief Runs once before exiting
    virtual void OnShutdown() = 0;
    /// @brief Runs once per frame
    virtual void Update(float deltaTime) = 0;
    /// @brief Runs at the fixed time. Runs in the main thread after Update
    virtual void FixedUpdate(float fixedDeltaTime) = 0;
    /// @brief Runs at the voxel fixed time
    /// @warning Runs on another thread. Beware of race conditions
    virtual void VoxelUpdate(float deltaTime) = 0;
    /// @brief Runs once per frame, after Update. Should be only used to render custom elements
    virtual void Render(glm::mat4 voxelProjection, glm::mat4 viewProjection) = 0;

    /// @brief Runs once at the start, Should be used to insert voxels into the VoxelRegistry
    virtual void RegisterVoxels() = 0;
    /// @brief Runs once at the start, Should be used to insert voxel objects into the VoxelObjectRegistry.
    /// Runs after `IGame::RegisterVoxels`
    virtual void RegisterVoxelObjects() = 0;

    /// @brief Called when the mouse is scrolled. Runs once per frame before Update
    virtual void OnMouseScroll(int yOffset) = 0;
    /// @brief Called when a mouse button is pressed down. Runs once per frame before Update
    virtual void OnMouseButtonDown(int button) = 0;
    /// @brief Called when a mouse button is released. Runs once per frame before Update
    virtual void OnMouseButtonUp(int button) = 0;
    /// @brief Called when the mouse is moved. Runs once per frame before Update
    /// @param x New mouse x position
    /// @param y New mouse y position
    virtual void OnMouseMove(int x, int y) = 0;
    /// @brief Called when a keyboard key is pressed down. Runs once per frame before Update
    virtual void OnKeyboardDown(int key) = 0;
    /// @brief Called when a keyboard key is released. Runs once per frame before Update
    virtual void OnKeyboardUp(int key) = 0;

    /// @brief Called when the window is resized. Runs once per frame before Update
    virtual void OnWindowResize(int newX, int newY) = 0;
};

// FIXME: default should be 1/30.0 but its acting wierdly with voxelFixedDeltaTime

namespace Config{
    enum class EnabledEngineFeatures{
        NONE = 0,
        HEAT_SIMULATION = 1 << 0,
        PRESSURE_SIMULATION = 1 << 1,
        CHEMICAL_REACTIONS = 1 << 2,
        ALL = HEAT_SIMULATION | PRESSURE_SIMULATION | CHEMICAL_REACTIONS
    };

    inline EnabledEngineFeatures operator|(EnabledEngineFeatures a, EnabledEngineFeatures b) {
        return static_cast<EnabledEngineFeatures>(static_cast<int>(a) | static_cast<int>(b));
    }
    inline EnabledEngineFeatures operator&(EnabledEngineFeatures a, EnabledEngineFeatures b) {
        return static_cast<EnabledEngineFeatures>(static_cast<int>(a) & static_cast<int>(b));
    }
    inline EnabledEngineFeatures operator~(EnabledEngineFeatures a) {
        return static_cast<EnabledEngineFeatures>(~static_cast<int>(a));
    }
    inline EnabledEngineFeatures& operator|=(EnabledEngineFeatures& a, EnabledEngineFeatures b) {
        a = a | b;
        return a;
    }
    inline EnabledEngineFeatures& operator&=(EnabledEngineFeatures& a, EnabledEngineFeatures b) {
        a = a & b;
        return a;
    }
    
    struct EngineConfig{
        RGB backgroundColor = RGB(0, 0, 0);
        bool vsync = true;
        bool automaticLoadingOfChunksInView = true;
        bool disableGPUSimulations = false;
        float fixedDeltaTime = 3/30.0;
        float voxelFixedDeltaTime = 1/30.0;

        bool pauseVoxelSimulation = false;
        EnabledEngineFeatures enabledFeatures = EnabledEngineFeatures::ALL;
    };
}

class GameEngine
{
private:
    Config::EngineConfig config;
    IGame *currentGame = nullptr;

    SDL_GLContext glContext;
    SDL_Event windowEvent;
    Uint64 LastFrameEndTime = SDL_GetPerformanceCounter();

    std::thread simulationThread;

    bool pauseVoxelSimulation = false;

    // Mouse position in screen coordinates
    Vec2f mousePos;

    VoxelObject *player = nullptr;

    float fixedUpdateTimer = 0;
    std::atomic<float> voxelUpdateTimer = 0;

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

    void Initialize(const Config::EngineConfig& config);
public:
    static GameRenderer* renderer;
    static GamePhysics* physics;
    static GameEngine* instance;

    static constexpr int MAX_FRAME_RATE = 60;
    float fixedDeltaTime;
    float voxelFixedDeltaTime;

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

    /// @return Mouse pos in screen space coordinates
    Vec2f GetMousePos() const { return mousePos; }

    ChunkMatrix chunkMatrix;

    GameEngine();

    void Run(IGame& game, const Config::EngineConfig& config);

    void SetPauseVoxelSimulation(bool pause);
    bool IsVoxelSimulationPaused() const { return pauseVoxelSimulation; }

    void SetPlayer(VoxelObject *player);
    VoxelObject *GetPlayer() const { return player; };
    ~GameEngine();

    Volume::Chunk* LoadChunkAtPosition(Vec2i pos);

    // disallow copying
    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
};