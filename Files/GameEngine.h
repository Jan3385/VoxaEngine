#pragma once

#include <SDL2/SDL.h>
#include "Math/Vector.h"
#include "World/Chunk.h"

class GameEngine
{
private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Event windowEvent;

    Uint64 FrameStartTime;

    float fixedUpdateTimer = 0;

    void m_initVariables();
    void m_initWindow();
    void m_OnKeyboardInput(SDL_Event event);
    void m_OnMouseButtonDown(SDL_MouseButtonEvent event);
    void m_FixedUpdate();
public:
    static constexpr int MAX_FRAME_RATE = 60;
    static constexpr float fixedDeltaTime = 1/30.0;
    bool running = true;
    float deltaTime = 1/60;    // time between frames in seconds
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
};