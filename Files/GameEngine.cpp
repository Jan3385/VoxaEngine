#include "GameEngine.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <thread>

using namespace std;

GameEngine::GameEngine()
{
    this->Camera = AABB(Vec2f(0, 0), Vec2f(640.0/Volume::Chunk::RENDER_VOXEL_SIZE, 480.0/Volume::Chunk::RENDER_VOXEL_SIZE));

    SDL_Init(SDL_INIT_EVERYTHING);
    if (TTF_Init() == -1) {
        std::cerr << "Error initializing SDL_ttf: " << TTF_GetError() << std::endl;
    }

    m_initVariables();
    m_initWindow();
}

GameEngine::~GameEngine()
{
    TTF_CloseFont(this->basicFont);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void GameEngine::StartFrame()
{
    this->FrameStartTime = SDL_GetPerformanceCounter();
}

void GameEngine::EndFrame()
{
    //cout << "FPS: " << this->FPS << endl;
    //cout << "Delta Time: " << this->deltaTime << endl;
    //cout << "Wait time " << ((1000.0 / MAX_FRAME_RATE) - deltaTime*1000) << endl;
    SDL_Delay(max(((1000.0 / MAX_FRAME_RATE) - deltaTime*1000), 0.0));

    Uint64 FrameEndTime = SDL_GetPerformanceCounter();

    float FrameTime = (FrameEndTime - this->FrameStartTime) / (float)SDL_GetPerformanceFrequency();

    this->deltaTime = FrameTime;
    this->FPS = 1.0f / FrameTime;
}

void GameEngine::Update()
{
    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();

    fixedUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        m_FixedUpdate();
        fixedUpdateTimer -= fixedDeltaTime;
    }

    if (MovementKeysHeld[0])
        this->MoveCamera(this->Camera.corner + Vec2f(0, -2));
    if (MovementKeysHeld[1])
        this->MoveCamera(this->Camera.corner + Vec2f(0, 2));
    if (MovementKeysHeld[2])
        this->MoveCamera(this->Camera.corner + Vec2f(-2, 0));
    if (MovementKeysHeld[3])
        this->MoveCamera(this->Camera.corner + Vec2f(2, 0));
}

void GameEngine::m_UpdateGridSegment(int pass)
{
    for (auto& chunk : chunkMatrix.GridSegmented[pass]) {
        if(chunk->ShouldChunkDelete(Camera))
        {
            chunkMatrix.DeleteChunk(chunk->GetPos());
            continue;
        }

        if (chunk->updateVoxelsNextFrame)
            chunk->UpdateVoxels(&this->chunkMatrix);
    }
}
void GameEngine::m_FixedUpdate()
{
    for(int i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            if(chunk->lastCheckedCountDown > 0 ) chunk->lastCheckedCountDown -= 1;
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
            case SDL_WINDOWEVENT:
                switch (this->windowEvent.window.event)
                {
                case SDL_WINDOWEVENT_RESIZED:
                    Camera.size = Vec2f(
                        this->windowEvent.window.data1/Volume::Chunk::RENDER_VOXEL_SIZE, 
                        this->windowEvent.window.data2/Volume::Chunk::RENDER_VOXEL_SIZE);
                    break;
                }
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

    //Draw logic TODO: make parallel
    for(int i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            if(chunk->GetAABB().Overlaps(Camera))
                chunk->Render(*this->renderer, GetCameraPos()*(-1*Volume::Chunk::RENDER_VOXEL_SIZE));
        }
    }

    chunkMatrix.RenderParticles(*this->renderer, GetCameraPos()*(-1*Volume::Chunk::RENDER_VOXEL_SIZE));	

    //fps counter
    SDL_Color color = { 255, 255, 255 };
    SDL_Surface* surface = TTF_RenderText_Solid(basicFont, to_string(FPS).c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = { 0, 0, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Update window
    SDL_RenderPresent( renderer );
}

Vec2f GameEngine::GetCameraPos() const
{
    return Vec2f(Camera.corner);
}

void GameEngine::MoveCamera(Vec2f pos)
{
    this->Camera.corner = pos;

    //Spawn chunks that are in the view but don´t exits
    Vec2i cameraChunkPos = chunkMatrix.WorldToChunkPosition(Vec2f(Camera.corner));
    int ChunksHorizontal = (Camera.size.getX() / Volume::Chunk::CHUNK_SIZE) + 1;
    int ChunksVertical = ceil(Camera.size.getY() / Volume::Chunk::CHUNK_SIZE) + 1;

    vector<thread> threads;

    for (int x = 0; x < ChunksHorizontal; ++x) {
        for (int y = 0; y < ChunksVertical; ++y) {
            Vec2i chunkPos = cameraChunkPos + Vec2i(x, y);
            threads.emplace_back(&GameEngine::m_LoadChunkInView, this, chunkPos);
        }
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
void GameEngine::m_LoadChunkInView(Vec2i pos)
{
    if(!chunkMatrix.IsValidChunkPosition(pos)) return;
    if(chunkMatrix.GetChunkAtChunkPosition(pos)) return;

    chunkMatrix.GenerateChunk(pos);
    return;
}

void GameEngine::m_initVariables()
{
    this->basicFont = TTF_OpenFont("Fonts/RobotoFont.ttf", 24);
    MoveCamera(Vec2f(0, 0));
    //constexpr int chunkGenerationSize = 10;
    //this->chunkMatrix.Grid.resize(chunkGenerationSize);
    //for (int i = 0; i < this->chunkMatrix.Grid.size(); ++i) {
    //	this->chunkMatrix.Grid[i].resize(chunkGenerationSize);
    //}

    //for (int x = 0; x < 10; ++x) {
    //	for (int y = 0; y < 10; ++y) {
    //		this->chunkMatrix.GenerateChunk(Vec2i(x, y));
    //	}
    //}
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
    Vec2f offset = this->Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
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
