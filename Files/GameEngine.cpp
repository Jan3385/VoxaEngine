#include "GameEngine.h"
#include <iostream>
#include <mutex>
#include <cstring>
#include <algorithm>

#include "Shader/ChunkShader.h"
#include "Registry/GameObjectRegistry.h"

using namespace std;

bool GameEngine::placeUnmovableSolidVoxels = false;
int GameEngine::placementRadius = 5;
int GameEngine::placeVoxelAmount = 20;
bool GameEngine::MovementKeysHeld[4] = {false, false, false, false};
GameEngine* GameEngine::instance = nullptr;

GameEngine::GameEngine()
{
    this->physics = new GamePhysics();

    this->renderer = new GameRenderer(&glContext);

    Registry::VoxelRegistry::RegisterVoxels();
    Registry::VoxelRegistry::CloseRegistry();

    Registry::GameObjectRegistry::RegisterObjects();
    Registry::GameObjectRegistry::CloseRegistry();
}

void GameEngine::Initialize(){
    this->Player = new Game::Player(&this->chunkMatrix, Registry::GameObjectRegistry::GetProperties("Player")->voxelData);
    physics->physicsObjects.push_back(this->Player);
}

GameEngine::~GameEngine()
{
    this->running = false;
    if (simulationThread.joinable())
        simulationThread.join();

    delete this->Player;

    SDL_GL_DeleteContext(glContext);

    delete this->renderer;

    delete this->physics;

    chunkMatrix.cleanup();
}

void GameEngine::StartFrame()
{
    this->FrameStartTime = SDL_GetPerformanceCounter();

    //SDL_Delay(max(((1000.0 / MAX_FRAME_RATE) - deltaTime*1000), 0.0));
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
    this->Player->UpdatePlayer(this->chunkMatrix, this->deltaTime);

    this->chunkMatrix.voxelMutex.lock();
    // Run physics simulation
    physics->Step(this->deltaTime, this->chunkMatrix);

    // update voxelobjects rotation
    for(VoxelObject* object : chunkMatrix.voxelObjects) {
        if(object->IsEnabled()) {
            object->UpdateRotatedVoxelBuffer();
        }
    }

    for(PhysicsObject* object : physics->physicsObjects) {
        if(object->IsEnabled()) {
            object->UpdatePhysicsEffects(chunkMatrix, deltaTime);
        }
    }
    this->chunkMatrix.voxelMutex.unlock();


    fixedUpdateTimer += deltaTime;
    simulationUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        m_FixedUpdate();
        fixedUpdateTimer -= fixedDeltaTime;
    }

    if(fixedUpdateTimer > fixedDeltaTime*2.5f)
        std::cout << "Fixed update timer is too high: " << fixedUpdateTimer << std::endl;
    
    if(simulationUpdateTimer > simulationFixedDeltaTime*2.5f)
        std::cout << "Simulation update timer is too high: " << simulationUpdateTimer << std::endl;
}
void GameEngine::m_UpdateGridVoxel(int pass)
{    
    #pragma omp parallel for
    for (size_t i = 0; i < chunkMatrix.GridSegmented[pass].size(); ++i) {
        auto& chunk = chunkMatrix.GridSegmented[pass][i];

        chunk->UpdateVoxels(&this->chunkMatrix);
        
        chunk->dirtyRect.Update();
    }
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

        chunkMatrix.voxelMutex.lock();
        // Update game objects
        for (auto it = chunkMatrix.voxelObjects.begin(); it != chunkMatrix.voxelObjects.end(); ) {
            VoxelObject* voxelObject = *it;
            if (voxelObject->IsEnabled()) {
                if (!voxelObject->Update(chunkMatrix)) {
                    it = chunkMatrix.voxelObjects.erase(it);

                    PhysicsObject* physicsObject = dynamic_cast<PhysicsObject*>(voxelObject);
                    if (physicsObject) {
                        this->physics->physicsObjects.remove(physicsObject);
                    }

                    delete voxelObject;
                    continue;
                }
            }
            ++it;
        }

        //Reset voxels to default pre-simulation state
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

        //delete all chunks marked for deletion
        for(int32_t i = static_cast<size_t>(chunkMatrix.Grid.size()) - 1; i >= 0; --i){
            if(chunkMatrix.Grid[i]->ShouldChunkDelete(this->Player->Camera))
            {
                chunkMatrix.DeleteChunk(chunkMatrix.Grid[i]->GetPos());
            }
        }

        //Voxel update logic
        for(uint8_t i = 0; i < 4; ++i)
        {
            m_UpdateGridVoxel(i);
        }

        // Update colliders for all chunks
        for(size_t i = 0; i < chunkMatrix.Grid.size(); ++i) {
            if(chunkMatrix.Grid[i]->dirtyColliders)
                physics->Generate2DCollidersForChunk(chunkMatrix.Grid[i]);
        }

        chunkMatrix.voxelMutex.unlock();

        chunkMatrix.UpdateParticles();
    }
}
void GameEngine::m_FixedUpdate()
{
    this->chunkMatrix.voxelMutex.lock();
    // Run heat and pressure simulation
    this->chunkMatrix.RunGPUSimulations();
    this->chunkMatrix.voxelMutex.unlock();
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
                    this->mousePos.x = this->windowEvent.motion.x;
                    this->mousePos.y = this->windowEvent.motion.y;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    m_OnMouseButtonDown(this->windowEvent.button);
                    break;
                case SDL_MOUSEWHEEL:
                    this->placementRadius += this->windowEvent.wheel.y;
                    this->placementRadius = std::clamp(this->placementRadius, 0, 10);
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
                    glViewport(0, 0, 
                        this->windowEvent.window.data1, 
                        this->windowEvent.window.data2
                    );
                    this->WindowSize.x = this->windowEvent.window.data1;
                    this->WindowSize.y = this->windowEvent.window.data2;
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
                {
                Vec2f worldMousePos = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), this->Player->Camera.corner);
                Registry::CreateGameObject("Barrel", worldMousePos, &chunkMatrix, this->physics);
                }
                break;
            case SDLK_z:
                {
                Vec2f worldMousePos2 = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), this->Player->Camera.corner);
                Registry::CreateGameObject("Ball", worldMousePos2, &chunkMatrix, this->physics);
                }
                break;
            case SDLK_u:
                {
                Vec2f worldMousePos3 = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), this->Player->Camera.corner);
                Registry::CreateGameObject("Crate", worldMousePos3, &chunkMatrix, this->physics);
                }
                break;
            }
            break;
        }
    }
}

void GameEngine::Render()
{
    chunkMatrix.voxelMutex.lock();
    renderer->Render(chunkMatrix, this->mousePos);
    chunkMatrix.voxelMutex.unlock();
}

void GameEngine::StartSimulationThread()
{
    this->simulationThread = std::thread(&GameEngine::m_SimulationThread, this);
}

void GameEngine::StopSimulationThread()
{
    std::cout << "Stopping simulation thread..." << std::endl;
    this->running = false;
    simulationThread.join();
    std::cout << "Simulation thread stopped." << std::endl;
}

Volume::Chunk* GameEngine::LoadChunkInView(Vec2i pos)
{
    if(!chunkMatrix.IsValidChunkPosition(pos)) return nullptr;
    if(chunkMatrix.GetChunkAtChunkPosition(pos)) return nullptr;

    chunkMatrix.chunkCreationMutex.lock();
    Volume::Chunk* chunk = chunkMatrix.GenerateChunk(pos);
    GameEngine::instance->renderer->chunkCreateBuffer.push_back(chunk);
    chunkMatrix.chunkCreationMutex.unlock();
    return chunk;
}

void GameEngine::m_OnKeyboardInput(SDL_KeyboardEvent event)
{

}

void GameEngine::m_OnMouseButtonDown(SDL_MouseButtonEvent event)
{
    chunkMatrix.voxelMutex.lock();
    
    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        if(this->Player->gunEnabled)
            this->Player->FireGun(this->chunkMatrix);
        else
            this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, this->placeVoxelType, this->Player->Camera.corner, Volume::Temperature(this->placeVoxelTemperature));
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 15, this->Player->Camera.corner);
    }
    chunkMatrix.voxelMutex.unlock();
}
