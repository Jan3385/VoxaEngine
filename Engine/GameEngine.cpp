#include "GameEngine.h"
#include <iostream>
#include <mutex>
#include <cstring>
#include <algorithm>

#include "Shader/ChunkShader.h"
#include "Registry/VoxelObjectRegistry.h"

using namespace std;

bool GameEngine::MovementKeysHeld[4] = {false, false, false, false};

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
}

void GameEngine::Initialize(const Config::EngineConfig& config){
    if(GameEngine::instance != nullptr){
        throw std::runtime_error("GameEngine is already initialized! Cannot run more than one instance.");
    }

    this->chunkMatrix = new ChunkMatrix();
    this->chunkMatrix->isActive = true;

    GameEngine::physics = new GamePhysics();

    GameEngine::renderer = new GameRenderer(&glContext, config.backgroundColor, config.automaticLoadingOfChunksInView);

    this->fixedDeltaTime = config.fixedDeltaTime;
    this->voxelFixedDeltaTime = config.voxelFixedDeltaTime;

    this->automaticLoadingOfChunksFromEvents = config.automaticLoadingOfChunksFromEvents;
    this->pauseVoxelSimulation = config.pauseVoxelSimulation;
    this->runHeatSimulation     =   (config.enabledFeatures & Config::EnabledEngineFeatures::HEAT_SIMULATION)       != Config::EnabledEngineFeatures::NONE;
    this->runPressureSimulation =   (config.enabledFeatures & Config::EnabledEngineFeatures::PRESSURE_SIMULATION)   != Config::EnabledEngineFeatures::NONE;
    this->runChemicalReactions  =   (config.enabledFeatures & Config::EnabledEngineFeatures::CHEMICAL_REACTIONS)    != Config::EnabledEngineFeatures::NONE;
    this->consoleTimerWarnings = config.consoleTimerWarnings;

    GameEngine::instance = this;
    GameEngine::renderer->SetVSYNC(config.vsync);

    this->chunkMatrix->Initialize(config.disableGPUSimulations);

    //TODO: just a temp fix to stop fixed and simulation time from running at the same time, blocking each other
    this->fixedUpdateTimer = -fixedDeltaTime / 2.0f;

    this->running = true;
    this->config = config;
}

void GameEngine::Run(IGame &game, const Config::EngineConfig& config)
{
    this->Initialize(config);
    this->currentGame = &game;

    Registry::VoxelRegistry::RegisterVoxels(this->currentGame);
    Registry::VoxelRegistry::CloseRegistry();

    Registry::VoxelObjectRegistry::RegisterObjects(this->currentGame);
    Registry::VoxelObjectRegistry::CloseRegistry();

    game.OnInitialize();

    GameEngine::instance->StartSimulationThread(game);

    //Main game loop
    while (this->running)
    {
        this->Update(game);

        this->Render();

        this->EndFrame();
    }

    game.OnShutdown();
}

void GameEngine::VoxelSimulationStep()
{
    // Update game objects
    for (auto it = chunkMatrix->voxelObjects.begin(); it != chunkMatrix->voxelObjects.end(); ) {
        VoxelObject* voxelObject = *it;
        if (voxelObject->IsEnabled()) {
            if (!voxelObject->Update(*chunkMatrix)) {
                it = chunkMatrix->voxelObjects.erase(it);

                PhysicsObject* physicsObject = dynamic_cast<PhysicsObject*>(voxelObject);
                if (physicsObject) {
                    GameEngine::instance->chunkMatrix->physicsObjects.remove(physicsObject);
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
        for (auto& chunk : chunkMatrix->GridSegmented[i]) {
            // decrease lastCheckedCountDown, this slowly kills unused chunks
            if(chunk->lastCheckedCountDown > 0 ) chunk->lastCheckedCountDown -= 1;

            if (!chunk->dirtyRect.IsEmpty())
                chunk->SIM_ResetVoxelUpdateData();
        }
    }

    //delete all chunks marked for deletion
    for(int32_t i = static_cast<size_t>(chunkMatrix->Grid.size()) - 1; i >= 0; --i){
        if(chunkMatrix->Grid[i]->ShouldChunkDelete(GameEngine::renderer->GetCameraAABB()))
            chunkMatrix->DeleteChunk(chunkMatrix->Grid[i]->GetPos());
    }

    //Voxel update logic
    for(uint8_t i = 0; i < 4; ++i) UpdateGridVoxel(i);

    // Update colliders for all chunks
    for(size_t i = 0; i < chunkMatrix->Grid.size(); ++i) {
        if(chunkMatrix->Grid[i]->dirtyColliders)
            physics->Generate2DCollidersForChunk(chunkMatrix->Grid[i]);
    }

    chunkMatrix->UpdateParticles();
}

void GameEngine::SetPauseVoxelSimulation(bool pause)
{
    this->pauseVoxelSimulation = pause;

    if(!this->pauseVoxelSimulation)
        this->voxelUpdateTimer = 0;
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

    chunkMatrix->cleanup();
    chunkMatrix->isActive = false;
    delete chunkMatrix;

    Registry::VoxelRegistry::CleanupRegistry();
}

void GameEngine::EndFrame()
{
    Uint64 FrameEndTime = SDL_GetPerformanceCounter();

    float FrameTime = (FrameEndTime - this->LastFrameEndTime) / (float)SDL_GetPerformanceFrequency();

    this->deltaTime = FrameTime;
    this->FPS = 1.0f / FrameTime;

    if(this->FPSHistory.size() >= AVG_FPS_SIZE_COUNT)
        this->FPSHistory.pop_back();

    this->FPSHistory.push_front(this->FPS);

    this->avgFPS = 0;
    for(auto& fps : this->FPSHistory)
        this->avgFPS += fps;
    this->avgFPS /= this->FPSHistory.size();

    this->LastFrameEndTime = FrameEndTime;
}

void GameEngine::Update(IGame& game)
{
    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();
    
    this->chunkMatrix->voxelMutex.lock();
    
    // Run physics simulation
    physics->Step(this->deltaTime, *this->chunkMatrix);

    // update voxelobjects rotation
    for(VoxelObject* object : chunkMatrix->voxelObjects) {
        if(object->IsEnabled()) {
            object->UpdateRotatedVoxelBuffer();
        }
    }

    //Physics
    for(PhysicsObject* object : chunkMatrix->physicsObjects) {
        if(object->IsEnabled()) {
            object->UpdatePhysicsEffects(*chunkMatrix, deltaTime);
        }
    }
    this->chunkMatrix->voxelMutex.unlock();

    game.Update(this->deltaTime);

    // Fixed update
    fixedUpdateTimer += deltaTime;
    voxelUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        FixedUpdate(game);
        game.FixedUpdate(this->fixedDeltaTime);
        fixedUpdateTimer -= fixedDeltaTime;
    }

    // Initialize any chunks that are not yet initialized
    while (!chunkMatrix->newUninitializedChunks.empty()) {
        auto chunk = chunkMatrix->newUninitializedChunks.front();
        chunkMatrix->newUninitializedChunks.pop();

        if (!chunk->IsInitialized()) {
            chunk->InitializeBuffers();
        }
    }

    // Swap chunk matrices if a new one was set
    if(this->futureChunkMatrix){
        this->chunkMatrix->voxelMutex.lock();
        this->chunkMatrix->chunkCreationMutex.lock();

        this->chunkMatrix->isActive = false;
        this->futureChunkMatrix->isActive = true;
        this->chunkMatrix = this->futureChunkMatrix;
        this->futureChunkMatrix = nullptr;

        this->chunkMatrix->voxelMutex.unlock();
        this->chunkMatrix->chunkCreationMutex.unlock();
    }

    if(fixedUpdateTimer > fixedDeltaTime*2.5f && consoleTimerWarnings)
        std::cout << "Fixed update timer is too high: " << fixedUpdateTimer << std::endl;

    if(voxelUpdateTimer > voxelFixedDeltaTime*2.5f && consoleTimerWarnings && !this->pauseVoxelSimulation)
        std::cout << "Voxel Simulation update timer is too high: " << voxelUpdateTimer << std::endl;
}
void GameEngine::UpdateGridVoxel(int pass)
{    
    #pragma omp parallel for
    for (size_t i = 0; i < chunkMatrix->GridSegmented[pass].size(); ++i) {
        auto& chunk = chunkMatrix->GridSegmented[pass][i];

        chunk->UpdateVoxels(this->chunkMatrix);
        
        chunk->dirtyRect.Update();
    }
}
void GameEngine::SimulationThread(IGame& game)
{
    while (this->running)
    {
        if (voxelUpdateTimer < voxelFixedDeltaTime || this->pauseVoxelSimulation) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep for a bit to avoid busy waiting
            continue;
        }
        voxelUpdateTimer -= voxelFixedDeltaTime;

        chunkMatrix->voxelMutex.lock();

        this->VoxelSimulationStep();

        chunkMatrix->voxelMutex.unlock();

        game.VoxelUpdate(this->voxelFixedDeltaTime);
    }
}
void GameEngine::FixedUpdate(IGame& game)
{
    this->chunkMatrix->chunkCreationMutex.lock();
    this->chunkMatrix->voxelMutex.lock();

    // Run heat and pressure simulation
    this->chunkMatrix->RunGPUSimulations();

    this->chunkMatrix->voxelMutex.unlock();
    this->chunkMatrix->chunkCreationMutex.unlock();
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
                    currentGame->OnMouseButtonDown(this->windowEvent.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    currentGame->OnMouseButtonUp(this->windowEvent.button.button);
                    break;
                case SDL_MOUSEWHEEL:
                    currentGame->OnMouseScroll(this->windowEvent.wheel.y);
                    break;
            }
        }

        //keyboard
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
                }
                currentGame->OnKeyboardDown(this->windowEvent.key.keysym.sym);
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
                    GameEngine::renderer->SetCameraSize(
                        Vec2f(
                            this->windowEvent.window.data1 / Volume::Chunk::RENDER_VOXEL_SIZE,
                            this->windowEvent.window.data2 / Volume::Chunk::RENDER_VOXEL_SIZE
                    ));

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
            }
            this->currentGame->OnKeyboardUp(this->windowEvent.key.keysym.sym);
            break;
        }
    }
}

void GameEngine::Render()
{
    chunkMatrix->voxelMutex.lock();
    GameEngine::renderer->Render(*chunkMatrix, this->mousePos, this->currentGame);
    chunkMatrix->voxelMutex.unlock();
}

void GameEngine::StartSimulationThread(IGame& game)
{
    this->simulationThread = std::thread(&GameEngine::SimulationThread, this, std::ref(game));
}

ChunkMatrix *GameEngine::GetActiveChunkMatrix()
{
    if(!this->chunkMatrix){
        throw std::runtime_error("Active chunk matrix is not yet set! (nullptr)");
    }

    return this->chunkMatrix;
}

/// @brief Gives the signal to change the active chunk matrix. The change will be applied at the end of this update call
/// @param matrix Matrix to set as active
/// @return Previous active chunk matrix
/// @warning The old chunk matrix will *not* be deleted. *You* are responsible for managing its memory
ChunkMatrix *GameEngine::SetActiveChunkMatrix(ChunkMatrix *matrix)
{
    if(matrix == nullptr){
        std::cerr << "Warning: Tried to set active chunk matrix to nullptr!" << std::endl;
        return this->chunkMatrix;
    }
    this->futureChunkMatrix = matrix;
    return this->chunkMatrix;
}

/// @brief Loads and generates a chunk at the specified chunk position and adds it to the world
/// @param pos position in chunk coordinates
/// @return pointer to the loaded chunk or nullptr if failed
Volume::Chunk* GameEngine::LoadChunkAtPosition(Vec2i pos)
{
    if(!chunkMatrix->IsValidChunkPosition(pos)) return nullptr;
    if(chunkMatrix->GetChunkAtChunkPosition(pos)) return nullptr;

    Volume::Chunk* chunk = chunkMatrix->GenerateChunk(pos);
    return chunk;
}