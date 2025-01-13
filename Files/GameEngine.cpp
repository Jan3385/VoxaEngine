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

    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();

    fixedUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        m_FixedUpdate();
        fixedUpdateTimer -= fixedDeltaTime;
    }
}
void GameEngine::m_FixedUpdate()
{
    for (auto& row : chunkMatrix.Grid) {
        for (auto& chunk : row) {
            if (chunk->updateVoxelsNextFrame)
                chunk->ResetVoxelUpdateData(&this->chunkMatrix);
        }
    }

    for (int i = 0; i < 4; ++i) {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            if (chunk->updateVoxelsNextFrame)
                chunk->UpdateVoxels(&this->chunkMatrix);
        }
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
            case SDL_KEYDOWN:
                m_OnKeyboardInput(this->windowEvent);
                break;
            case SDL_MOUSEBUTTONDOWN:
                m_OnMouseButtonDown(this->windowEvent.button);
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
    for (const auto& row : chunkMatrix.Grid) {
        for (const auto& chunk : row) {
            chunk->Render(*this->renderer);
        }
    }

    chunkMatrix.RenderParticles(*this->renderer);	

    // Update window
    SDL_RenderPresent( renderer );
}

void GameEngine::m_initVariables()
{
    constexpr int chunkGenerationSize = 10;
    this->chunkMatrix.Grid.resize(chunkGenerationSize);
    for (int i = 0; i < this->chunkMatrix.Grid.size(); ++i) {
    	this->chunkMatrix.Grid[i].resize(chunkGenerationSize);
    }

    for (int x = 0; x < this->chunkMatrix.Grid.size(); ++x) {
    	for (int y = 0; y < this->chunkMatrix.Grid[0].size(); ++y) {
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

void GameEngine::m_OnKeyboardInput(SDL_Event event)
{
}

void GameEngine::m_OnMouseButtonDown(SDL_MouseButtonEvent event)
{
    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, Volume::VoxelType::Sand);
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.RemoveVoxelAtMousePosition(this->mousePos);
        break;
    case SDL_BUTTON_MIDDLE:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 10);
        break;
    case SDL_BUTTON_X1:
        this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, Volume::VoxelType::Water);
        std::cout << "X1" << std::endl;
        break;
    case SDL_BUTTON_X2:
        this->chunkMatrix.PlaceParticleAtMousePosition(this->mousePos, Volume::VoxelType::Fire, 0, 2);
        break;
    }
}
