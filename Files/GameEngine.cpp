#include "GameEngine.h"
#include <iostream>
#include <mutex>
#include <future>
#include <cstring>

#include "Shader/ChunkShader.h"
#include "Registry/GameObjectRegistry.h"

using namespace std;

bool GameEngine::placeUnmovableSolidVoxels = false;
int GameEngine::placementRadius = 5;
int GameEngine::placeVoxelAmount = 1;
bool GameEngine::MovementKeysHeld[4] = {false, false, false, false};
GameEngine* GameEngine::instance = nullptr;

GameEngine::GameEngine()
{
    this->physics = new GamePhysics();

    Registry::VoxelRegistry::RegisterVoxels();
    this->renderer = new GameRenderer(&glContext);
    
    ChunkShader::InitializeBuffers();
    ChunkShader::InitializeComputeShaders();
}

GameEngine::~GameEngine()
{
    this->running = false;
    if (simulationThread.joinable())
        simulationThread.join();

    SDL_GL_DeleteContext(glContext);

    delete this->renderer;

    delete this->physics;

    delete this->Player;

    chunkMatrix.cleanup();
}

void GameEngine::StartFrame()
{
    this->FrameStartTime = SDL_GetPerformanceCounter();

    SDL_Delay(max(((1000.0 / MAX_FRAME_RATE) - deltaTime*1000), 0.0));
}

void GameEngine::EndFrame()
{
    Uint64 FrameEndTime = SDL_GetPerformanceCounter();

    float FrameTime = (FrameEndTime - this->FrameStartTime) / (float)SDL_GetPerformanceFrequency();

    this->deltaTime = FrameTime;
    this->FPS = 1.0f / FrameTime;

    if(this->FPSHistory.size() >= AVG_FPS_SIZE_COUNT)
        this->FPSHistory.pop_back();

    this->FPSHistory.push_front(this->FPS);

    this->avgFPS = 0;
    for(auto& fps : this->FPSHistory)
        this->avgFPS += fps;
    this->avgFPS /= this->FPSHistory.size();
}

void GameEngine::Update()
{
    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();

    //Update Player
    this->Player->Update(this->chunkMatrix, this->deltaTime);

    fixedUpdateTimer += deltaTime;
    simulationUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        m_FixedUpdate();
        fixedUpdateTimer -= fixedDeltaTime;
    }

    if(fixedUpdateTimer > fixedDeltaTime*2)
        std::cout << "Fixed update timer is too high: " << fixedUpdateTimer << std::endl;
    
    if(simulationUpdateTimer > simulationFixedDeltaTime*2)
        std::cout << "Simulation update timer is too high: " << simulationUpdateTimer << std::endl;
}
void GameEngine::m_UpdateGridVoxel(int pass)
{
    chunkMatrix.voxelMutex.lock();
    //delete all chunks marked for deletion
    for(int32_t i = static_cast<int32_t>(chunkMatrix.GridSegmented[pass].size()) - 1; i >= 0; --i){
        if(chunkMatrix.GridSegmented[pass][i]->ShouldChunkDelete(this->Player->Camera))
        {
            chunkMatrix.DeleteChunk(chunkMatrix.GridSegmented[pass][i]->GetPos());
            continue;
        }
    }
    chunkMatrix.voxelMutex.unlock();

    chunkMatrix.voxelMutex.lock();
    #pragma omp parallel for
    for (size_t i = 0; i < chunkMatrix.GridSegmented[pass].size(); ++i) {
        auto& chunk = chunkMatrix.GridSegmented[pass][i];
        if (!chunk->dirtyRect.IsEmpty()) {
            chunk->UpdateVoxels(&this->chunkMatrix);
        }
        chunk->dirtyRect.Update();
    }
    chunkMatrix.voxelMutex.unlock();
}
void GameEngine::m_SimulationThread()
{
    while (this->running)
    {
        if (simulationUpdateTimer < simulationFixedDeltaTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep for a bit to avoid busy waiting
            continue;
        }
            

        simulationUpdateTimer -= simulationFixedDeltaTime;

        //Reset voxels to default pre-simulation state
        chunkMatrix.voxelMutex.lock();
        #pragma omp parallel for
        for(uint8_t i = 0; i < 4; ++i)
        {
            for (auto& chunk : chunkMatrix.GridSegmented[i]) {
                // decrease lastCheckedCountDown, this slowly kills unused chunks
                if(chunk->lastCheckedCountDown > 0 ) chunk->lastCheckedCountDown -= 1;

                if (!chunk->dirtyRect.IsEmpty())
                    chunk->SIM_ResetVoxelUpdateData();
            }
        }
        chunkMatrix.voxelMutex.unlock();

        //Voxel update logic
        for(uint8_t i = 0; i < 4; ++i)
        {
            m_UpdateGridVoxel(i);
        }

        chunkMatrix.UpdateParticles();
    }
}
void GameEngine::m_FixedUpdate()
{
    std::lock_guard<std::mutex> lock(this->chunkMatrix.voxelMutex);
    // Run heat and pressure simulation
    ChunkShader::RunChunkShaders(chunkMatrix);
}

void GameEngine::PollEvents()
{
    ImGuiIO& io = ImGui::GetIO();
    while (SDL_PollEvent(&this->windowEvent) == 1)
    {
        ImGui_ImplSDL2_ProcessEvent(&this->windowEvent);
           
        //mouse
        if(!io.WantCaptureMouse){
            switch (this->windowEvent.type){
                case SDL_MOUSEMOTION:
                    this->mousePos.x(this->windowEvent.motion.x);
                    this->mousePos.y(this->windowEvent.motion.y);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    m_OnMouseButtonDown(this->windowEvent.button);
                    break;
            }
        }
        //keyboard (except keyUp)
        if(!io.WantCaptureKeyboard){
            switch (this->windowEvent.type)
            {
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
            }
        }

        //other events (including keyUp)
        switch (this->windowEvent.type)
        {
        case SDL_QUIT:
            this->running = false;
            break;
        case SDL_WINDOWEVENT:
            switch (this->windowEvent.window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                    this->Player->Camera.size = Vec2f(
                        this->windowEvent.window.data1/Volume::Chunk::RENDER_VOXEL_SIZE, 
                        this->windowEvent.window.data2/Volume::Chunk::RENDER_VOXEL_SIZE);
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
            case SDLK_t:
                Vec2f worldMousePos = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), this->Player->Camera.corner*Volume::Chunk::RENDER_VOXEL_SIZE);
                Registry::CreateGameObject(&chunkMatrix, renderer->LoadTexture("Textures/Barrel.bmp"),
                    worldMousePos
                );
                break;
            }
            break;
        }
    }
}

void GameEngine::Render()
{
    chunkMatrix.voxelMutex.lock();
    //Render
    renderer->Render(chunkMatrix, this->mousePos);
    chunkMatrix.voxelMutex.unlock();
}

void GameEngine::StartSimulationThread()
{
    this->simulationThread = std::thread(&GameEngine::m_SimulationThread, this);
}

void GameEngine::LoadChunkInView(Vec2i pos)
{
    if(!chunkMatrix.IsValidChunkPosition(pos)) return;
    if(chunkMatrix.GetChunkAtChunkPosition(pos)) return;

    chunkMatrix.GenerateChunk(pos);
    return;
}

void GameEngine::m_OnKeyboardInput(SDL_KeyboardEvent event)
{

}

void GameEngine::m_OnMouseButtonDown(SDL_MouseButtonEvent event)
{
    chunkMatrix.voxelMutex.lock();
    Vec2f offset = this->Player->Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        if(this->Player->gunEnabled)
            this->Player->FireGun(this->chunkMatrix);
        else
            this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, this->placeVoxelType, offset, Volume::Temperature(this->placeVoxelTemperature));
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 15, offset);
    }
    chunkMatrix.voxelMutex.unlock();
}
