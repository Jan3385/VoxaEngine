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

bool GameEngine::NoClip = false;
bool GameEngine::GunEnabled = false;

GameEngine* GameEngine::instance = nullptr;
GameRenderer* GameEngine::renderer = nullptr;
GamePhysics* GameEngine::physics = nullptr;

GameEngine::GameEngine()
{
    if(GameEngine::instance != nullptr){
        throw std::runtime_error("GameEngine is already initialized! Cannot run more than one instance.");
    }

    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));

    GameEngine::physics = new GamePhysics();

    GameEngine::renderer = new GameRenderer(&glContext);

    Registry::VoxelRegistry::RegisterVoxels();
    Registry::VoxelRegistry::CloseRegistry();

    Registry::GameObjectRegistry::RegisterObjects();
    Registry::GameObjectRegistry::CloseRegistry();
}

void GameEngine::Initialize(const EngineConfig& config){
    if(GameEngine::instance != nullptr){
        throw std::runtime_error("GameEngine is already initialized! Cannot run more than one instance.");
    }

    GameEngine::instance = this;

    this->running = true;
    this->config = config;
}

void GameEngine::Run(IGame &game, const EngineConfig& config)
{
    this->Initialize(config);
    this->currentGame = &game;
    game.OnInitialize();

    GameEngine::instance->StartSimulationThread(game);

    //Main game loop
    while (this->running)
    {
        this->StartFrame();

        this->Update(game);

        this->Render();
        game.Render();

        this->EndFrame();
    }

    game.OnShutdown();
}

void GameEngine::SetPlayer(VoxelObject *player)
{
    this->player = player;
}

GameEngine::~GameEngine()
{
    this->running = false;
    
    simulationThread.join();

    SDL_GL_DeleteContext(glContext);

    if(GameEngine::renderer){
        delete GameEngine::renderer;
        GameEngine::renderer = nullptr;
    }

    if(GameEngine::physics){
        delete GameEngine::physics;
        GameEngine::physics = nullptr;
    }

    chunkMatrix.cleanup();
}

void GameEngine::StartFrame()
{
    this->FrameStartTime = SDL_GetPerformanceCounter();
}

void GameEngine::EndFrame()
{
    //TODO: instead of startframe get the previous frame's end time
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

void GameEngine::Update(IGame& game)
{
    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();

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

    game.Update(this->deltaTime);

    fixedUpdateTimer += deltaTime;
    simulationUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        FixedUpdate(game);
        game.FixedUpdate(this->fixedDeltaTime);
        fixedUpdateTimer -= fixedDeltaTime;
    }

    if(fixedUpdateTimer > fixedDeltaTime*2.5f)
        std::cout << "Fixed update timer is too high: " << fixedUpdateTimer << std::endl;
    
    if(simulationUpdateTimer > simulationFixedDeltaTime*2.5f)
        std::cout << "Simulation update timer is too high: " << simulationUpdateTimer << std::endl;
}
void GameEngine::UpdateGridVoxel(int pass)
{    
    #pragma omp parallel for
    for (size_t i = 0; i < chunkMatrix.GridSegmented[pass].size(); ++i) {
        auto& chunk = chunkMatrix.GridSegmented[pass][i];

        chunk->UpdateVoxels(&this->chunkMatrix);
        
        chunk->dirtyRect.Update();
    }
}
void GameEngine::SimulationThread(IGame& game)
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
                        GameEngine::physics->physicsObjects.remove(physicsObject);
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
            if(chunkMatrix.Grid[i]->ShouldChunkDelete(GameEngine::renderer->Camera))
            {
                chunkMatrix.DeleteChunk(chunkMatrix.Grid[i]->GetPos());
            }
        }

        //Voxel update logic
        for(uint8_t i = 0; i < 4; ++i)
        {
            UpdateGridVoxel(i);
        }

        // Update colliders for all chunks
        for(size_t i = 0; i < chunkMatrix.Grid.size(); ++i) {
            if(chunkMatrix.Grid[i]->dirtyColliders)
                physics->Generate2DCollidersForChunk(chunkMatrix.Grid[i]);
        }

        chunkMatrix.voxelMutex.unlock();

        chunkMatrix.UpdateParticles();

        game.PhysicsUpdate(this->simulationFixedDeltaTime);
    }
}
void GameEngine::FixedUpdate(IGame& game)
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
                    currentGame->OnMouseMove(this->mousePos.x, this->mousePos.y);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    OnMouseButtonDown(this->windowEvent.button);
                    currentGame->OnMouseButtonDown(this->windowEvent.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    currentGame->OnMouseButtonUp(this->windowEvent.button.button);
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
                    GameEngine::MovementKeysHeld[0] = true;
                    break;
                case SDLK_s:
                    GameEngine::MovementKeysHeld[1] = true;
                    break;
                case SDLK_a:
                    GameEngine::MovementKeysHeld[2] = true;
                    break;
                case SDLK_d:
                    GameEngine::MovementKeysHeld[3] = true;
                    break;
                default:
                    OnKeyboardInput(this->windowEvent.key);
                    break;
                }
                break;
            }
            currentGame->OnKeyboardDown(this->windowEvent.key.keysym.sym);
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
                    glViewport(0, 0, 
                        this->windowEvent.window.data1, 
                        this->windowEvent.window.data2
                    );
                    this->WindowSize.x = this->windowEvent.window.data1;
                    this->WindowSize.y = this->windowEvent.window.data2;
                    this->currentGame->OnWindowResize(this->WindowSize.x, this->WindowSize.y);
                break;
            }
            break;
        case SDL_KEYUP:
            switch (this->windowEvent.key.keysym.sym)
            {
            case SDLK_w:
                GameEngine::MovementKeysHeld[0] = false;
                break;
            case SDLK_s:
                GameEngine::MovementKeysHeld[1] = false;
                break;
            case SDLK_a:
                GameEngine::MovementKeysHeld[2] = false;
                break;
            case SDLK_d:
                GameEngine::MovementKeysHeld[3] = false;
                break;
            case SDLK_t:
                {
                Vec2f worldMousePos = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), GameEngine::renderer->Camera.corner);
                Registry::CreateGameObject("Barrel", worldMousePos, &chunkMatrix, GameEngine::physics);
                }
                break;
            case SDLK_z:
                {
                Vec2f worldMousePos2 = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), GameEngine::renderer->Camera.corner);
                Registry::CreateGameObject("Ball", worldMousePos2, &chunkMatrix, GameEngine::physics);
                }
                break;
            case SDLK_u:
                {
                Vec2f worldMousePos3 = chunkMatrix.MousePosToWorldPos(Vec2f(this->mousePos), GameEngine::renderer->Camera.corner);
                Registry::CreateGameObject("Crate", worldMousePos3, &chunkMatrix, GameEngine::physics);
                }
                break;
            }
            this->currentGame->OnKeyboardUp(this->windowEvent.key.keysym.sym);
            break;
        }
    }
}

void GameEngine::Render()
{
    chunkMatrix.voxelMutex.lock();
    GameEngine::renderer->Render(chunkMatrix, this->mousePos, config.backgroundColor);
    chunkMatrix.voxelMutex.unlock();
}

void GameEngine::StartSimulationThread(IGame& game)
{
    this->simulationThread = std::thread(&GameEngine::SimulationThread, this, std::ref(game));
}

Volume::Chunk* GameEngine::LoadChunkInView(Vec2i pos)
{
    if(!chunkMatrix.IsValidChunkPosition(pos)) return nullptr;
    if(chunkMatrix.GetChunkAtChunkPosition(pos)) return nullptr;

    chunkMatrix.chunkCreationMutex.lock();
    Volume::Chunk* chunk = chunkMatrix.GenerateChunk(pos);
    GameEngine::renderer->chunkCreateBuffer.push_back(chunk);
    chunkMatrix.chunkCreationMutex.unlock();
    return chunk;
}

void GameEngine::OnKeyboardInput(SDL_KeyboardEvent event)
{
    switch (event.keysym.sym)
    {
    case SDLK_F1:
        GameEngine::renderer->fullImGui = !GameEngine::renderer->fullImGui;
    default:
        break;
    }
}

//TODO: game should be handling this
void GameEngine::OnMouseButtonDown(SDL_MouseButtonEvent event)
{
    chunkMatrix.voxelMutex.lock();
    
    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        if(!GameEngine::GunEnabled)
            this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, this->placeVoxelType, GameEngine::renderer->Camera.corner, Volume::Temperature(this->placeVoxelTemperature));
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 15, GameEngine::renderer->Camera.corner);
    }
    chunkMatrix.voxelMutex.unlock();
}
