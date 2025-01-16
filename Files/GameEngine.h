#pragma once

#include <SDL2/SDL.h>
#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Chunk.h"

class GameEngine
{
private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Event windowEvent;
    Uint64 FrameStartTime;

    float fixedUpdateTimer = 0;

    TTF_Font* basicFont;
    AABB Camera;

    void m_initVariables();
    void m_initWindow();
    void m_OnKeyboardInput(SDL_KeyboardEvent event);
    void m_OnMouseButtonDown(SDL_MouseButtonEvent event);
    void m_FixedUpdate();
    void m_UpdateGridSegment(int pass);
    void m_LoadChunkInView(Vec2i pos);
public:
    static constexpr int MAX_FRAME_RATE = 60;
    static constexpr float fixedDeltaTime = 1/30.0;
    bool running = true;
    float deltaTime = 1/60.0;    // time between frames in seconds
    float FPS = 60;
    
    Vec2f mousePos;

    Vec2f PlayerVelocity;
    bool MovementKeysHeld[4] = {false, false, false, false};

    ChunkMatrix chunkMatrix;

    GameEngine();
    ~GameEngine();
    void StartFrame();
    void EndFrame();
    void Update();
    void PollEvents();
    void Render();

    Vec2f GetCameraPos() const;
    void MoveCamera(Vec2f pos);
};