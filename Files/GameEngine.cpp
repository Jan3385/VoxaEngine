#include "GameEngine.h"
#include <iostream>

using namespace std;

GameEngine::GameEngine()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    m_initVariables();
    m_initWindow();
}

GameEngine::~GameEngine()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GameEngine::StartFrame()
{
    this->FrameStartTime = SDL_GetPerformanceCounter();
}

void GameEngine::EndFrame()
{
    SDL_Delay(1000 / MAX_FRAME_RATE - deltaTime);

    Uint64 FrameEndTime = SDL_GetPerformanceCounter();

    float FrameTime = (FrameEndTime - this->FrameStartTime) / (float)SDL_GetPerformanceFrequency();

    this->deltaTime = FrameTime;
    this->FPS = 1.0f / FrameTime;
}

void GameEngine::Update()
{
    //cout << "FPS: " << this->FPS << endl;
}

void GameEngine::PollEvents()
{
    while (SDL_PollEvent(&this->windowEvent) == 1)
    {
        switch (this->windowEvent.type)
        {
            case SDL_QUIT:
                this->running = false;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                // :)
                break;
            case SDL_MOUSEMOTION:
                this->mousePos.x(this->windowEvent.motion.x);
                this->mousePos.y(this->windowEvent.motion.y);
                break;
            case SDL_KEYDOWN:
                m_OnKeyboardInput(this->windowEvent);
                break;
        }
    }
}

void GameEngine::Render()
{
    //SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_ADD ); // Switch to additive 

    SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
    SDL_RenderClear( renderer ); // Clear the screen to solid white

    // Update window
    SDL_RenderPresent( renderer );
}

void GameEngine::m_initVariables()
{
    
}

void GameEngine::m_initWindow()
{
    if(SDL_CreateWindowAndRenderer(640, 480, SDL_WindowFlags::SDL_WINDOW_RESIZABLE, &window, &renderer) != 0){
        cout << "Error with window creation: " << SDL_GetError() << endl;
        exit(1);
    }
}

void GameEngine::m_OnKeyboardInput(SDL_Event event)
{
}

