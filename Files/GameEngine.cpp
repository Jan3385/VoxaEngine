#include "GameEngine.h"
#include <iostream>
#include <thread>

using namespace std;

GameEngine::GameEngine()
{
    this->Camera = AABB(Vec2f(0, 0), Vec2f(640, 480));

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

    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();

    fixedUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        m_FixedUpdate();
        fixedUpdateTimer -= fixedDeltaTime;
    }

    if (MovementKeysHeld[0])
        this->Camera.corner.y(this->Camera.corner.getY() - 5);
    if (MovementKeysHeld[1])
        this->Camera.corner.y(this->Camera.corner.getY() + 5);
    if (MovementKeysHeld[2])
        this->Camera.corner.x(this->Camera.corner.getX() - 5);
    if (MovementKeysHeld[3])
        this->Camera.corner.x(this->Camera.corner.getX() + 5);
}

void GameEngine::m_UpdateGridSegment(int pass)
{
    for (auto& chunk : chunkMatrix.GridSegmented[pass]) {
        if (chunk->updateVoxelsNextFrame)
            chunk->UpdateVoxels(&this->chunkMatrix);
    }
}

void GameEngine::m_FixedUpdate()
{
    for(int i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            if (chunk->updateVoxelsNextFrame)
                chunk->ResetVoxelUpdateData(&this->chunkMatrix);
        }
    }

    for(int i = 0; i < 4; ++i)
    {
        m_UpdateGridSegment(i);
    }

    chunkMatrix.UpdateParticles();
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
            case SDL_MOUSEBUTTONDOWN:
                m_OnMouseButtonDown(this->windowEvent.button);
                break;
            case SDL_KEYDOWN:
                switch (this->windowEvent.key.keysym.sym)
                {
                case SDLK_w:
                    MovementKeysHeld[0] = true;
                    break;
                case SDLK_s:
                    MovementKeysHeld[1] = true;
                    break;
                case SDLK_a:
                    MovementKeysHeld[2] = true;
                    break;
                case SDLK_d:
                    MovementKeysHeld[3] = true;
                    break;
                default:
                    m_OnKeyboardInput(this->windowEvent.key);
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (this->windowEvent.key.keysym.sym)
                {
                case SDLK_w:
                    MovementKeysHeld[0] = false;
                    break;
                case SDLK_s:
                    MovementKeysHeld[1] = false;
                    break;
                case SDLK_a:
                    MovementKeysHeld[2] = false;
                    break;
                case SDLK_d:
                    MovementKeysHeld[3] = false;
                    break;
                }
                break;
        }
    }
}

void GameEngine::Render()
{
    //SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_ADD ); // Switch to additive 

    SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
    SDL_RenderClear( renderer ); // Clear the screen to solid white

    //Draw logic
    for(int i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            chunk->Render(*this->renderer, Camera.corner*-1);
        }
    }

    chunkMatrix.RenderParticles(*this->renderer, Camera.corner*-1);	

    // Update window
    SDL_RenderPresent( renderer );
}

void GameEngine::m_initVariables()
{
    constexpr int chunkGenerationSize = 10;
    //this->chunkMatrix.Grid.resize(chunkGenerationSize);
    //for (int i = 0; i < this->chunkMatrix.Grid.size(); ++i) {
    //	this->chunkMatrix.Grid[i].resize(chunkGenerationSize);
    //}

    for (int x = 0; x < 10; ++x) {
    	for (int y = 0; y < 10; ++y) {
    		this->chunkMatrix.GenerateChunk(Vec2i(x, y));
    	}
    }
}

void GameEngine::m_initWindow()
{
    if(SDL_CreateWindowAndRenderer(640, 480, SDL_WindowFlags::SDL_WINDOW_RESIZABLE, &window, &renderer) != 0){
        cout << "Error with window creation: " << SDL_GetError() << endl;
        exit(1);
    }
}

void GameEngine::m_OnKeyboardInput(SDL_KeyboardEvent event)
{

}

void GameEngine::m_OnMouseButtonDown(SDL_MouseButtonEvent event)
{
    Vec2f offset = this->Camera.corner;
    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, Volume::VoxelType::Sand, offset);
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.RemoveVoxelAtMousePosition(this->mousePos, offset);
        break;
    case SDL_BUTTON_MIDDLE:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 10, offset);
        break;
    case SDL_BUTTON_X1:
        this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, Volume::VoxelType::Water, offset);
        break;
    case SDL_BUTTON_X2:
        this->chunkMatrix.PlaceParticleAtMousePosition(this->mousePos, Volume::VoxelType::Fire, offset, 0, 2);
        break;
    }
}
