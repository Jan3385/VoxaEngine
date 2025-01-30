#pragma once

#include <SDL.h>
#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Chunk.h"
#include "Player/Player.h"

class GameEngine
{
private:
    SDL_GLContext glContext;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Event windowEvent;
    Uint64 FrameStartTime;

    bool oddHeatUpdatePass = false;

    float fixedUpdateTimer = 0;

    TTF_Font* basicFont;

    bool debugRendering = false;
    bool showHeatAroundCursor = false;

    std::string placeVoxelType = "Sand";
    float placeVoxelTemperature = 21.0;

    void m_initVariables();
    void m_initWindow();
    void m_OnKeyboardInput(SDL_KeyboardEvent event);
    void m_OnMouseButtonDown(SDL_MouseButtonEvent event);
    void m_FixedUpdate();
    void m_UpdateGridVoxel(int pass);
    void m_UpdateGridHeat(int pass);
    void m_RenderIMGUI();

    void m_toggleDebugRendering();
public:
    static GameEngine* instance;

    static constexpr int MAX_FRAME_RATE = 60;
    static constexpr float fixedDeltaTime = 1/30.0;

    static bool placeUnmovableSolidVoxels;
    static int placementRadius;

    static bool MovementKeysHeld[4]; //W, S, A, D

    Game::Player Player;

    bool running = true;
    float deltaTime = 1/60.0;    // time between frames in seconds
    float FPS = 60;
    
    Vec2f mousePos;

    ChunkMatrix chunkMatrix;

    GameEngine();
    ~GameEngine();
    void StartFrame();
    void EndFrame();
    void Update();
    void PollEvents();
    void Render();

    void LoadChunkInView(Vec2i pos);
};