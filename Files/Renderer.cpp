#include "Renderer.h"

#include "GameEngine.h"

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

#include "Math/AABB.h"
#include "Player/Player.h"

GameRenderer::GameRenderer()
{
}

GameRenderer::GameRenderer(SDL_GLContext *glContext)
{
    std::cout << "Initializing SDL2" << std::endl;
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        std::cerr << "Error initializing SDL2: " << SDL_GetError() << std::endl;
    }
    if (TTF_Init() == -1) {
        std::cerr << "Error initializing SDL_ttf: " << TTF_GetError() << std::endl;
    }

    IMGUI_CHECKVERSION();
    
    this->basicFont = TTF_OpenFont("Fonts/RobotoFont.ttf", 24);

    if(SDL_CreateWindowAndRenderer(800, 600, SDL_WindowFlags::SDL_WINDOW_RESIZABLE | SDL_WindowFlags::SDL_WINDOW_OPENGL, &r_window, &r_renderer) != 0){
        std::cout << "Error with window creation: " << SDL_GetError() << std::endl;
        exit(1);
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(
        r_window,
        r_renderer
    );

    ImGui_ImplSDLRenderer2_Init(r_renderer);

    SDL_SetWindowTitle(r_window, "VoxaEngine");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    *glContext = SDL_GL_CreateContext(r_window);
    if (!glContext) {
        std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
        exit(1);
    }

    //Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error initializing GLEW: " << glewGetErrorString(err) << std::endl;
    }
}

GameRenderer::~GameRenderer()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    TTF_CloseFont(this->basicFont);

    SDL_DestroyWindow(r_window);
    SDL_DestroyRenderer(r_renderer);

    TTF_Quit();
    SDL_Quit();
}

void GameRenderer::Render(ChunkMatrix &chunkMatrix, Vec2i mousePos)
{
    

    //SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_ADD ); // Switch to additive 
    SDL_SetRenderDrawColor(r_renderer, 26, 198, 255, 255 );
    SDL_RenderClear( r_renderer ); // Clear the screen to solid white

    //Player rendering
    Game::Player player = GameEngine::instance->Player;
    player.Render(r_renderer);

    //chunk rendering
    std::vector<std::thread> threads;
    std::vector<std::pair<SDL_Surface*, SDL_Rect>> renderData;
    std::mutex renderDataMutex;

    AABB cameraAABB = player.Camera.Expand(1);
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
                            (player.Camera.corner.getX() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))-padding.getX(),
                        static_cast<int>(chunk->GetPos().getY() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + 
                            (player.Camera.corner.getY() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))-padding.getY(),
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
        SDL_Texture* chunkTexture = SDL_CreateTextureFromSurface(r_renderer, chunkSurface);
    
        SDL_RenderCopy(r_renderer, chunkTexture, NULL, &data.second);
    
        SDL_DestroyTexture(chunkTexture);
    }

    //rendering particles
    chunkMatrix.RenderParticles(*this->r_renderer, player.Camera.corner*(-1*Volume::Chunk::RENDER_VOXEL_SIZE));	

    if(showHeatAroundCursor){
        SDL_SetRenderDrawBlendMode( r_renderer, SDL_BLENDMODE_BLEND );
        Vec2f offset = player.Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
        Vec2f voxelPos = ChunkMatrix::MousePosToWorldPos(mousePos, offset);

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
                        static_cast<int>(localPos.getX() * Volume::Chunk::RENDER_VOXEL_SIZE + 
                            (chunkPos.getX() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + 
                                (player.Camera.corner.getX() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))),

                        static_cast<int>(localPos.getY() * Volume::Chunk::RENDER_VOXEL_SIZE + 
                            (chunkPos.getY() * Volume::Chunk::CHUNK_SIZE * Volume::Chunk::RENDER_VOXEL_SIZE + 
                                (player.Camera.corner.getY() * Volume::Chunk::RENDER_VOXEL_SIZE * -1))),
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
                    SDL_SetRenderDrawColor(r_renderer, 
                        color.r, 0, color.b, alpha);
                    SDL_RenderFillRect(r_renderer, &rect);
                }
            }
        }
        SDL_SetRenderDrawBlendMode( r_renderer, SDL_BLENDMODE_NONE );
    }

    if(debugRendering) {
        //fps counter
        SDL_Color color = { 255, 255, 255, 255 };
        SDL_Surface* surface = TTF_RenderText_Solid(basicFont, std::to_string(GameEngine::instance->FPS).c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(r_renderer, surface);
        SDL_Rect rect = { 0, 0, surface->w, surface->h };
        SDL_RenderCopy(r_renderer, texture, NULL, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    
        //info about the voxel under the mouse
        Vec2f offset = player.Camera.corner*(Volume::Chunk::RENDER_VOXEL_SIZE);
        Vec2f mouseWorldPos = ChunkMatrix::MousePosToWorldPos(mousePos, offset);
        auto voxel = chunkMatrix.VirtualGetAt(mouseWorldPos);
        if(voxel){
            SDL_Surface* surface = TTF_RenderText_Solid(basicFont, (std::to_string(voxel->temperature.GetCelsius()) + "deg C").c_str(), color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(r_renderer, surface);
            SDL_Rect rect = { 0, 20, surface->w, surface->h };
            SDL_RenderCopy(r_renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
            surface = TTF_RenderText_Solid(basicFont, voxel->id.c_str(), color);
            texture = SDL_CreateTextureFromSurface(r_renderer, surface);
            rect = { 0, 40, surface->w, surface->h };
            SDL_RenderCopy(r_renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }
    }

    //ImGui
    RenderIMGUI(chunkMatrix);

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), r_renderer);

    // Update window
    SDL_RenderPresent( r_renderer );
}
void GameRenderer::RenderIMGUI(ChunkMatrix &chunkMatrix)
{
    Game::Player player = GameEngine::instance->Player;

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("VoxaEngine Debug Panel");

    ImGui::Text("FPS: %lf", GameEngine::instance->FPS);
    
    const char* voxelTypeNames[] = {
        "Dirt", "Grass", "Stone", "Sand", "Oxygen",
        "Water", "Fire", "Plasma", "Carbon_Dioxide", "Iron", "Rust"
    };
    // Find the index of placeVoxelType in voxelTypeNames
    static int current_item = 0;
    for (int i = 0; i < IM_ARRAYSIZE(voxelTypeNames); ++i) {
        if (voxelTypeNames[i] == GameEngine::instance->placeVoxelType) {
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
                GameEngine::instance->placeVoxelType = voxelTypeNames[i];
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SliderInt("Placement Radius", &GameEngine::instance->placementRadius, 1, 10);
    ImGui::DragFloat("Placement Temperature", &GameEngine::instance->placeVoxelTemperature, 0.5f, -200.0f, 2500.0f);
    ImGui::Checkbox("Place Unmovable Solid Voxels", &GameEngine::instance->placeUnmovableSolidVoxels);
    if(ImGui::Button("Toggle Debug Rendering")) ToggleDebugRendering();
    ImGui::Checkbox("Show Heat Around Cursor", &showHeatAroundCursor);

    ImGui::Checkbox("No Clip", &player.NoClip);
    ImGui::Checkbox("Heat Simulation", &GameEngine::instance->runHeatSimulation);
    ImGui::DragFloat("Fixed Update speed", &GameEngine::instance->fixedDeltaTime, 0.05f, 1/30.0, 4);

    ImGui::Text("Loaded chunks: %lld", chunkMatrix.Grid.size());

    ImGui::End();

    ImGui::Render();
}

SDL_Texture *GameRenderer::LoadTexture(const char *path)
{
    SDL_Surface *surface = SDL_LoadBMP(path);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(r_renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void GameRenderer::ToggleDebugRendering()
{
    this->debugRendering = !this->debugRendering;

    //redraw all chunks
    for (auto& chunk : GameEngine::instance->chunkMatrix.Grid) {
        chunk->dirtyRender = true;
    }
}
