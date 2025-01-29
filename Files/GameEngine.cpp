#include "GameEngine.h"
#include <SDL_ttf.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <future>

using namespace std;

bool GameEngine::placeUnmovableSolidVoxels = false;
int GameEngine::placementRadius = 5;

GameEngine::GameEngine()
{
    VoxelRegistry::RegisterVoxels();

    this->Camera = AABB(Vec2f(0, 0), Vec2f(800.0/Volume::Chunk::RENDER_VOXEL_SIZE, 600.0/Volume::Chunk::RENDER_VOXEL_SIZE));

    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        std::cerr << "Error initializing SDL2: " << SDL_GetError() << std::endl;
    }
    if (TTF_Init() == -1) {
        std::cerr << "Error initializing SDL_ttf: " << TTF_GetError() << std::endl;
    }

    IMGUI_CHECKVERSION();

    m_initVariables();
    m_initWindow();
}

GameEngine::~GameEngine()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    TTF_CloseFont(this->basicFont);
    SDL_DestroyWindow(window);
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

void GameEngine::m_UpdateGridVoxel(int pass)
{
    //delete all chunks marked for deletion
    for(int i = chunkMatrix.GridSegmented[pass].size() - 1; i >= 0; --i){
        if(chunkMatrix.GridSegmented[pass][i]->ShouldChunkDelete(Camera))
        {
            chunkMatrix.DeleteChunk(chunkMatrix.GridSegmented[pass][i]->GetPos());
            continue;
        }
    }

    std::vector<std::thread> threads;
    for (auto& chunk : chunkMatrix.GridSegmented[pass]) {
        threads.push_back(std::thread([&]() {
            chunk->dirtyRect.Update();
            if (!chunk->dirtyRect.IsEmpty()){
                chunk->UpdateVoxels(&this->chunkMatrix);
            }
        }));
    }
    for (auto& thread : threads) {
        thread.join();
    }
}
void GameEngine::m_UpdateGridHeat(int pass)
{
    std::vector<std::thread> threads;
    for (auto& chunk : chunkMatrix.GridSegmented[pass]) {
        threads.push_back(std::thread([&]() {
            if (chunk->ShouldChunkCalculateHeat())
                chunk->UpdateHeat(&this->chunkMatrix, oddHeatUpdatePass);
        }));
    }
    for (auto& thread : threads) {
        thread.join();
    }
}
void GameEngine::m_FixedUpdate()
{
    #pragma omp parallel for collapse(2)
    for(int i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            if(chunk->lastCheckedCountDown > 0 ) chunk->lastCheckedCountDown -= 1;
            if (!chunk->dirtyRect.IsEmpty())
                chunk->ResetVoxelUpdateData(&this->chunkMatrix);
        }
    }

    //Voxel update logic
    for(int i = 0; i < 4; ++i)
    {
        m_UpdateGridVoxel(i);
    }

    //Heat update logic
    std::vector<std::thread> threads;
    for(int i = 0; i < 4; ++i)
    {
        threads.push_back(std::thread(&GameEngine::m_UpdateGridHeat, this, i));
    }
    oddHeatUpdatePass = !oddHeatUpdatePass;

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

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
                    Camera.size = Vec2f(
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
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    SDL_RenderClear( renderer ); // Clear the screen to solid white

    //chunk rendering
    std::vector<std::thread> threads;
    std::vector<std::pair<SDL_Surface*, SDL_Rect>> renderData;
    std::mutex renderDataMutex;
    for(int i = 0; i < 4; ++i)
    {
        threads.push_back(std::thread([&, i]{
            std::vector<std::pair<SDL_Surface*, SDL_Rect>> localData;
            for (auto& chunk : chunkMatrix.GridSegmented[i]) {
                if(chunk->GetAABB().Overlaps(Camera)){
                    SDL_Surface *chunkSurface = chunk->Render(this->debugRendering);

                    SDL_Rect rect = {
                        static_cast<int>(chunk->GetPos().getX() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + (GetCameraPos().getX() * Volume::Chunk::RENDER_VOXEL_SIZE * -1)),
                        static_cast<int>(chunk->GetPos().getY() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + (GetCameraPos().getY() * Volume::Chunk::RENDER_VOXEL_SIZE * -1)),
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
    chunkMatrix.RenderParticles(*this->renderer, GetCameraPos()*(-1*Volume::Chunk::RENDER_VOXEL_SIZE));	

    if(showHeatAroundCursor){
        SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
        Vec2f offset = this->Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
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
                        static_cast<int>(localPos.getX() * Volume::Chunk::RENDER_VOXEL_SIZE + (chunkPos.getX() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + (GetCameraPos().getX() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))),
                        static_cast<int>(localPos.getY() * Volume::Chunk::RENDER_VOXEL_SIZE + (chunkPos.getY() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + (GetCameraPos().getY() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))),
                        Volume::Chunk::RENDER_VOXEL_SIZE,
                        Volume::Chunk::RENDER_VOXEL_SIZE
                    };
                    float temperature = chunk->voxels[localPos.getX()][localPos.getY()]->temperature.GetCelsius();
                    // show temperature from -20 to 235 degrees (255 range)
                    if(temperature > 200) temperature = 200;
                    if(temperature < -20) temperature = -20;

                    temperature += 20;
                    RGB color = RGB(temperature, 0, 255 - temperature);
                    int alpha = distance < radius/1.4 ? 180 : 180 - (distance - radius/1.4) * 180 / (radius - radius/1.4);
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
        SDL_Color color = { 255, 255, 255 };
        SDL_Surface* surface = TTF_RenderText_Solid(basicFont, to_string(FPS).c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = { 0, 0, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    
        //info about the voxel under the mouse
        Vec2f offset = this->Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
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
    
    const char* voxelTypeNames[] = {
        "Dirt", "Grass", "Stone", "Sand", "Oxygen",
        "Water", "Fire", "Plasma", "Carbon_Dioxide", "Iron"
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

    ImGui::End();

    ImGui::Render();
}

void GameEngine::m_toggleDebugRendering()
{
    this->debugRendering = !this->debugRendering;

    //redraw all chunks
    for(int i = 0; i < 4; ++i)
    {
        for (auto& chunk : chunkMatrix.GridSegmented[i]) {
            chunk->dirtyRender = true;
        }
    }
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
    int ChunksHorizontal = ceil(Camera.size.getX() / Volume::Chunk::CHUNK_SIZE) + 1;
    int ChunksVertical = ceil(Camera.size.getY() / Volume::Chunk::CHUNK_SIZE) + 1;

    std::vector<Vec2i> chunksToLoad;
    for (int x = 0; x < ChunksHorizontal; ++x) {
        for (int y = 0; y < ChunksVertical; ++y) {
            Vec2i chunkPos = cameraChunkPos + Vec2i(x, y);
            if (!chunkMatrix.GetChunkAtChunkPosition(chunkPos)) {
                chunksToLoad.push_back(chunkPos);
            }
        }
    }

    std::vector<std::future<void>> futures;
    for (const auto& chunkPos : chunksToLoad) {
        futures.emplace_back(std::async(std::launch::async, &GameEngine::m_LoadChunkInView, this, chunkPos));
    }

    // Wait for all chunks to finish loading
    for (auto& future : futures) {
        future.get();
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
    if(SDL_CreateWindowAndRenderer(800, 600, SDL_WindowFlags::SDL_WINDOW_RESIZABLE, &window, &renderer) != 0){
        cout << "Error with window creation: " << SDL_GetError() << endl;
        exit(1);
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    //ImGui::StyleColorsDark();

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
        this->chunkMatrix.PlaceVoxelsAtMousePosition(this->mousePos, this->placeVoxelType, offset, Volume::Temperature(this->placeVoxelTemperature));
        break;
    case SDL_BUTTON_RIGHT:
        this->chunkMatrix.ExplodeAtMousePosition(this->mousePos, 15, offset);
    }
}
