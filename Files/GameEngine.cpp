#include "GameEngine.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <cstring>

using namespace std;

bool GameEngine::placeUnmovableSolidVoxels = false;
int GameEngine::placementRadius = 5;
int GameEngine::placeVoxelAmount = 1;
bool GameEngine::MovementKeysHeld[4] = {false, false, false, false};
GameEngine* GameEngine::instance = nullptr;

GameEngine::GameEngine()
{
    GameEngine::instance = this;
    VoxelRegistry::RegisterVoxels();

    this->renderer = new GameRenderer(&glContext);

    //compile compute shaders
    Volume::Chunk::computeShaderHeat_Program = m_compileComputeShader(Volume::Chunk::computeShaderHeat);
    Volume::VoxelElement::computeShaderPressure_Program = m_compileComputeShader(Volume::VoxelElement::computeShaderPressure);

    Player = Game::Player();
    Player.SetPlayerTexture(this->renderer->LoadTexture("Textures/Player.bmp"));
}

GameEngine::~GameEngine()
{
    SDL_GL_DeleteContext(glContext);

    delete this->renderer;

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
}

void GameEngine::Update()
{
    //Polls events - e.g. window close, keyboard input..
    this->PollEvents();

    //Update Player
    this->Player.Update(this->chunkMatrix, this->deltaTime);

    fixedUpdateTimer += deltaTime;
    if (fixedUpdateTimer >= fixedDeltaTime)
    {
        m_FixedUpdate();
        fixedUpdateTimer -= fixedDeltaTime;
    }
}

void GameEngine::m_UpdateGridVoxel(int pass)
{
    //delete all chunks marked for deletion
    for(int32_t i = static_cast<int32_t>(chunkMatrix.GridSegmented[pass].size()) - 1; i >= 0; --i){
        if(chunkMatrix.GridSegmented[pass][i]->ShouldChunkDelete(this->Player.Camera))
        {
            chunkMatrix.DeleteChunk(chunkMatrix.GridSegmented[pass][i]->GetPos());
            continue;
        }
    }

    std::vector<std::thread> threads;
    for (auto& chunk : chunkMatrix.GridSegmented[pass]) {
        threads.push_back(std::thread([&]() {
            if (!chunk->dirtyRect.IsEmpty()){
                chunk->UpdateVoxels(&this->chunkMatrix);
            }
            chunk->dirtyRect.Update();
        }));
    }
    for (auto& thread : threads) {
        thread.join();
    }
}
void GameEngine::m_FixedUpdate()
{
    //Reset voxels to default pre-simulation state
    #pragma omp parallel for
    for(uint8_t i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            if(chunk->lastCheckedCountDown > 0 ) chunk->lastCheckedCountDown -= 1;
            if (!chunk->dirtyRect.IsEmpty())
                chunk->ResetVoxelUpdateData();
        }
    }

    //Voxel update logic
    for(uint8_t i = 0; i < 4; ++i)
    {
        m_UpdateGridVoxel(i);
    }

    //Heat update logic
    if(runHeatSimulation) chunkMatrix.UpdateGridHeat(oddUpdatePass);

    //Pressure update logic
    if(runPressureSimulation) chunkMatrix.UpdateGridPressure(oddUpdatePass);
    oddUpdatePass = !oddUpdatePass;

    chunkMatrix.UpdateParticles();
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
                    this->Player.Camera.size = Vec2f(
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
            }
            break;
        }
    }
}

void GameEngine::Render()
{
    //Render
    renderer->Render(chunkMatrix, this->mousePos);
}

GLuint GameEngine::m_compileComputeShader(const char *shader)
{
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader, 1, &shader, NULL);
    glCompileShader(computeShader);

    GLint success;
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        std::cerr << "Error compiling compute shader: " << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Error linking compute shader program: " << infoLog << std::endl;
    }

    glDeleteShader(computeShader);

    return program;
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
    Vec2f offset = this->Player.Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, this->placeVoxelType, offset, Volume::Temperature(this->placeVoxelTemperature));
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 15, offset);
    }
}
