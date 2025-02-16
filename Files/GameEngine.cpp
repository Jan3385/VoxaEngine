#include "GameEngine.h"
#include <SDL_ttf.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <cstring>

using namespace std;

bool GameEngine::placeUnmovableSolidVoxels = false;
int GameEngine::placementRadius = 5;
bool GameEngine::MovementKeysHeld[4] = {false, false, false, false};
GameEngine* GameEngine::instance = nullptr;

GameEngine::GameEngine()
{
    GameEngine::instance = this;
    VoxelRegistry::RegisterVoxels();

    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        std::cerr << "Error initializing SDL2: " << SDL_GetError() << std::endl;
    }
    if (TTF_Init() == -1) {
        std::cerr << "Error initializing SDL_ttf: " << TTF_GetError() << std::endl;
    }

    IMGUI_CHECKVERSION();

    this->basicFont = TTF_OpenFont("Fonts/RobotoFont.ttf", 24);
    m_initWindow();

    //Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error initializing GLEW: " << glewGetErrorString(err) << std::endl;
    }

    Volume::Chunk::computeShaderHeat_Program = m_compileComputeShader(Volume::Chunk::computeShaderHeat);

    Player = Game::Player();
    Player.LoadPlayerTexture(renderer);
}

GameEngine::~GameEngine()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    TTF_CloseFont(this->basicFont);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);

    chunkMatrix.cleanup();

    TTF_Quit();
    SDL_Quit();
}

void GameEngine::StartFrame()
{
    this->FrameStartTime = SDL_GetPerformanceCounter();

    //cout << "FPS: " << this->FPS << endl;
    //cout << "Delta Time: " << this->deltaTime << endl;
    //cout << "Wait time " << ((1000.0 / MAX_FRAME_RATE) - deltaTime*1000) << endl;
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
    if(runHeatSimulation) chunkMatrix.UpdateGridHeat(oddHeatUpdatePass);
    oddHeatUpdatePass = !oddHeatUpdatePass;

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
    //SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_ADD ); // Switch to additive 
    SDL_SetRenderDrawColor( renderer, 26, 198, 255, 255 );
    SDL_RenderClear( renderer ); // Clear the screen to solid white

    //Player rendering
    this->Player.Render(renderer);

    //chunk rendering
    std::vector<std::thread> threads;
    std::vector<std::pair<SDL_Surface*, SDL_Rect>> renderData;
    std::mutex renderDataMutex;

    AABB cameraAABB = this->Player.Camera.Expand(1);
    for(uint8_t i = 0; i < 4; ++i)
    {
        threads.push_back(std::thread([&, i]{
            std::vector<std::pair<SDL_Surface*, SDL_Rect>> localData;
            for (auto& chunk : chunkMatrix.GridSegmented[i]) {
                if(chunk->GetAABB().Overlaps(cameraAABB)){
                    // wierd hack to fix seams between chunks
                    Vec2i padding = Vec2i(0, 0);
                    if(chunk->GetAABB().corner.getX() <= cameraAABB.corner.getX()+1) padding.x(1);
                    if (chunk->GetAABB().corner.getY() <= cameraAABB.corner.getY()+1) padding.y(1);

                    SDL_Surface *chunkSurface = chunk->Render(this->debugRendering);

                    SDL_Rect rect = {
                        static_cast<int>(chunk->GetPos().getX() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + 
                            (this->Player.Camera.corner.getX() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))-padding.getX(),
                        static_cast<int>(chunk->GetPos().getY() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + 
                            (this->Player.Camera.corner.getY() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))-padding.getY(),
                        Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE,
                        Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE
                    };

                    localData.push_back({chunkSurface, rect});
                }
            }
            std::lock_guard<std::mutex> lock(renderDataMutex);
            renderData.insert(renderData.end(), localData.begin(), localData.end());
        }));
    }
    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    for(auto& data : renderData){
        SDL_Surface* chunkSurface = data.first;
        SDL_Texture* chunkTexture = SDL_CreateTextureFromSurface(renderer, chunkSurface);
    
        SDL_RenderCopy(renderer, chunkTexture, NULL, &data.second);
    
        SDL_DestroyTexture(chunkTexture);
    }

    //rendering particles
    chunkMatrix.RenderParticles(*this->renderer, this->Player.Camera.corner*(-1*Volume::Chunk::RENDER_VOXEL_SIZE));	

    if(showHeatAroundCursor){
        SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
        Vec2f offset = this->Player.Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
        Vec2f voxelPos = ChunkMatrix::MousePosToWorldPos(this->mousePos, offset);

        constexpr int radius = 16;
        for(int x = -radius; x <= radius; ++x){
            for(int y = -radius; y <= radius; ++y){
                //render a circle around the cursor
                int distance = SDL_sqrt(x*x + y*y);
                if(distance > radius) continue;

                Vec2i pos = voxelPos + Vec2i(x, y);
                if(!chunkMatrix.IsValidWorldPosition(pos)) continue;
                Vec2i chunkPos = chunkMatrix.WorldToChunkPosition(Vec2f(pos));
                Volume::Chunk* chunk = chunkMatrix.GetChunkAtChunkPosition(chunkPos);
                if(chunk){
                    Vec2i localPos = pos - chunkPos*Volume::Chunk::CHUNK_SIZE;
                    SDL_Rect rect = {
                        static_cast<int>(localPos.getX() * Volume::Chunk::RENDER_VOXEL_SIZE + (chunkPos.getX() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + (this->Player.Camera.corner.getX() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))),
                        static_cast<int>(localPos.getY() * Volume::Chunk::RENDER_VOXEL_SIZE + (chunkPos.getY() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + (this->Player.Camera.corner.getY() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))),
                        Volume::Chunk::RENDER_VOXEL_SIZE,
                        Volume::Chunk::RENDER_VOXEL_SIZE
                    };
                    float temperature = chunk->voxels[localPos.getX()][localPos.getY()]->temperature.GetCelsius();
                    // show temperature from -20 to 235 degrees (255 range)
                    if(temperature > 200) temperature = 200;
                    if(temperature < -20) temperature = -20;

                    temperature += 20;
                    RGB color = RGB(temperature, 0, 255 - temperature);
                    uint8_t alpha = distance < radius/1.4 ? 180 : 180 - (distance - radius/1.4) * 180 / (radius - radius/1.4);
                    SDL_SetRenderDrawColor(renderer, 
                        color.r, 0, color.b, alpha);
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
        SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
    }

    if(debugRendering) {
        //fps counter
        SDL_Color color = { 255, 255, 255, 255 };
        SDL_Surface* surface = TTF_RenderText_Solid(basicFont, to_string(FPS).c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = { 0, 0, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    
        //info about the voxel under the mouse
        Vec2f offset = this->Player.Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
        Vec2f mouseWorldPos = ChunkMatrix::MousePosToWorldPos(this->mousePos, offset);
        auto voxel = chunkMatrix.VirtualGetAt(mouseWorldPos);
        if(voxel){
            SDL_Surface* surface = TTF_RenderText_Solid(basicFont, (to_string(voxel->temperature.GetCelsius()) + "deg C").c_str(), color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = { 0, 20, surface->w, surface->h };
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
            surface = TTF_RenderText_Solid(basicFont, voxel->id.c_str(), color);
            texture = SDL_CreateTextureFromSurface(renderer, surface);
            rect = { 0, 40, surface->w, surface->h };
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }

    //ImGui
    m_RenderIMGUI();

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    // Update window
    SDL_RenderPresent( renderer );
}
void GameEngine::m_RenderIMGUI()
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("VoxaEngine Debug Panel");

    ImGui::Text("FPS: %lf", this->FPS);
    
    const char* voxelTypeNames[] = {
        "Dirt", "Grass", "Stone", "Sand", "Oxygen",
        "Water", "Fire", "Plasma", "Carbon_Dioxide", "Iron", "Rust"
    };
    // Find the index of placeVoxelType in voxelTypeNames
    static int current_item = 0;
    for (int i = 0; i < IM_ARRAYSIZE(voxelTypeNames); ++i) {
        if (voxelTypeNames[i] == placeVoxelType) {
            current_item = i;
            break;
        }
    }

    if (ImGui::BeginCombo("Placement Voxel", voxelTypeNames[current_item]))
    {
        for (int i = 0; i < IM_ARRAYSIZE(voxelTypeNames); ++i)
        {
            bool is_selected = (current_item == i);
            if (ImGui::Selectable(voxelTypeNames[i], is_selected))
            {
                current_item = i;
                placeVoxelType = voxelTypeNames[i];
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SliderInt("Placement Radius", &placementRadius, 1, 10);
    ImGui::DragFloat("Placement Temperature", &placeVoxelTemperature, 0.5f, -200.0f, 2500.0f);
    ImGui::Checkbox("Place Unmovable Solid Voxels", &placeUnmovableSolidVoxels);
    if(ImGui::Button("Toggle Debug Rendering")) m_toggleDebugRendering();
    ImGui::Checkbox("Show Heat Around Cursor", &showHeatAroundCursor);

    ImGui::Checkbox("No Clip", &Player.NoClip);
    ImGui::Checkbox("Heat Simulation", &runHeatSimulation);
    ImGui::DragFloat("Fixed Update speed", &fixedDeltaTime, 0.05f, 1/30.0, 4);

    ImGui::Text("Loaded chunks: %lld", chunkMatrix.Grid.size());

    ImGui::End();

    ImGui::Render();
}

void GameEngine::m_toggleDebugRendering()
{
    this->debugRendering = !this->debugRendering;

    //redraw all chunks
    for(uint8_t i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            chunk->dirtyRender = true;
        }
    }
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

void GameEngine::m_initWindow()
{
    if(SDL_CreateWindowAndRenderer(800, 600, SDL_WindowFlags::SDL_WINDOW_RESIZABLE | SDL_WindowFlags::SDL_WINDOW_OPENGL, &window, &renderer) != 0){
        cout << "Error with window creation: " << SDL_GetError() << endl;
        exit(1);
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(
        window,
        renderer
    );
    
    ImGui_ImplSDLRenderer2_Init(renderer);

    SDL_SetWindowTitle(window, "VoxaEngine");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    this->glContext = SDL_GL_CreateContext(window);
    if (!this->glContext) {
        std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
        exit(1);
    }
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
