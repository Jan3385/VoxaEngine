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
#include "GameObject/Player.h"
#include "Physics/Physics.h"

#define AVG_FPS_SIZE_COUNT 25

struct IGame{
    virtual void OnInitialize() = 0;
    virtual void OnShutdown() = 0;
    virtual void Update() = 0;
    virtual void FixedUpdate() = 0;
    virtual void PhysicsUpdate() = 0;
    virtual void Render() = 0;

    virtual ~IGame() = default;
};

struct EngineConfig{
    RGBA backgroundColor;
};

class GameEngine
{
private:
    EngineConfig config;

    SDL_GLContext glContext;
    SDL_Event windowEvent;
    Uint64 FrameStartTime;
    
    std::thread simulationThread;

    float fixedUpdateTimer = 0;
    std::atomic<float> simulationUpdateTimer = 0;

    void m_OnKeyboardInput(SDL_KeyboardEvent event);
    void m_OnMouseButtonDown(SDL_MouseButtonEvent event);

    //Deletes old chunks and updates steps for voxel celluar automata simulation
    void m_UpdateGridVoxel(int pass);

    //Fixed update, Handles heat and pressure simulation
    void m_FixedUpdate(IGame& game);

    //Simulation thread, handles voxel simulation
    void m_SimulationThread(IGame& game);
public:
    GameRenderer* renderer;
    GamePhysics* physics;
    static GameEngine* instance;

    static constexpr int MAX_FRAME_RATE = 60;
    float fixedDeltaTime = 1/30.0;
    float simulationFixedDeltaTime = 1/30.0;

    static bool placeUnmovableSolidVoxels;
    static int placeVoxelAmount;
    static int placementRadius;

    static bool MovementKeysHeld[4]; //W, S, A, D

    GameEntities::Player *Player;

    bool running = true;
    float deltaTime = 1/60.0;    // time between frames in seconds

    float FPS = 60;
    float avgFPS = 60;
    std::list<float> FPSHistory = {};

    Vec2i WindowSize = {800, 600};    

    bool runHeatSimulation = true;
    bool runPressureSimulation = true;
    bool runChemicalReactions = true;

    std::string placeVoxelType = "Sand";
    float placeVoxelTemperature = 21.0;
    
    // Mouse position in screen coordinates
    Vec2f mousePos;

    ChunkMatrix chunkMatrix;

    GameEngine();

    void Initialize(const EngineConfig& config);
    void Run(IGame& game, const EngineConfig& config);
    ~GameEngine();
    void StartFrame();
    void EndFrame();
    void Update(IGame& game);
    void PollEvents();
    void Render();

    void StartSimulationThread(IGame& game);
    void StopSimulationThread();

    Volume::Chunk* LoadChunkInView(Vec2i pos);

    // disallow copying
    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
};