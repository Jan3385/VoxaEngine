#include "Renderer.h"

#include "GameEngine.h"

#include <iostream>
#include <vector>
#include <mutex>
#include <math.h>

#include <glm/gtc/matrix_transform.hpp>

#include "Math/AABB.h"
#include "GameObject/Player.h"

#include "World/Particle.h"
#include "World/ParticleGenerators/LaserParticleGenerator.h"

GameRenderer::GameRenderer()
{
    
}

GameRenderer::GameRenderer(SDL_GLContext *glContext)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        std::cerr << "Error initializing SDL2: " << SDL_GetError() << std::endl;
    }

    IMGUI_CHECKVERSION();

    r_window = SDL_CreateWindow("VoxaEngine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WindowFlags::SDL_WINDOW_OPENGL | SDL_WindowFlags::SDL_WINDOW_RESIZABLE
    );
    glViewport(0, 0, 800, 600);

    if(r_window == nullptr) {
        std::cout << "Error with window creation: " << SDL_GetError() << std::endl;
        exit(1);
    }

    *glContext = SDL_GL_CreateContext(r_window);
    if (!glContext) {
        std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
        exit(1);
    }
    r_GLContext = glContext;

    SDL_GL_SetSwapInterval(0); // Disable VSync

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(
        r_window,
        *glContext
    );
    ImGui_ImplOpenGL3_Init("#version 460");

    SDL_SetWindowTitle(r_window, "VoxaEngine");

    //Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error initializing GLEW: " << glewGetErrorString(err) << std::endl;
    }

    this->chunkRenderProgram = Shader::Shader(
        Shader::voxelArraySimulationVertexShader,
        Shader::voxelArraySimulationFragmentShader
    );
    this->particleRenderProgram = Shader::Shader(
        Shader::voxelParticleVertexShader,
        Shader::voxelParticleFragmentShader
    );
    this->closedShapeRenderProgram = Shader::Shader(
        Shader::closedShapeDrawVertexShader,
        Shader::closedShapeDrawFragmentShader
    );

    // Quad VBO setup ----
    float quad[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    glGenBuffers(1, &this->quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // --------------------

    glGenBuffers(1, &this->particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, this->particleVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    //Particle VAO setup ----
    glGenVertexArrays(1, &particleVAO);
    glBindVertexArray(particleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle::ParticleRenderData), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // instance attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle::ParticleRenderData), (void*)(sizeof(glm::vec2)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    // --------------

    // Closed shape VAO setup ----
    glGenVertexArrays(1, &closedShapeVAO);
    glGenBuffers(1, &closedShapeVBO);
    
    glBindVertexArray(closedShapeVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, closedShapeVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    this->fontRenderer.Initialize();
}

GameRenderer::~GameRenderer()
{
    // Cleanup OpenGL resources
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);
    glDeleteBuffers(1, &quadVBO);

    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(r_window);
    
    SDL_Quit();
}

void GameRenderer::Render(ChunkMatrix &chunkMatrix, Vec2i mousePos)
{
    // Enables Alpha Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disables Depth Testing
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.77f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Game::Player *player = GameEngine::instance->Player;

    glm::mat4 voxelProj = glm::ortho(
        player->Camera.corner.getX(), player->Camera.corner.getX() + player->Camera.size.getX(), 
        player->Camera.corner.getY() + player->Camera.size.getY(), player->Camera.corner.getY(),
        -1.0f, 1.0f
    );
    glm::mat4 screenProj = glm::ortho(
        0.0f, player->Camera.size.getX(), 
        player->Camera.size.getY(), 0.0f,
        -1.0f, 1.0f
    );

    for(auto& chunk : this->chunkCreateBuffer) {
        chunk->SetVBOData();
    }
    this->chunkCreateBuffer.clear();

    this->chunkRenderProgram.Use();
    this->chunkRenderProgram.SetMat4("projection", voxelProj);
    
    for (auto& chunk : chunkMatrix.Grid) {
        if(chunk->GetAABB().Overlaps(player->Camera)){
            chunk->Render(false);

            glBindVertexArray(chunk->VAO);
            glDrawArraysInstanced(
                GL_TRIANGLE_FAN, 0, 4, 
                Volume::Chunk::CHUNK_SIZE_SQUARED
            );
        }
    }

    if(chunkMatrix.particles.size() > 0){
        this->UpdateParticleVBO(chunkMatrix);
        this->particleRenderProgram.Use();
        this->particleRenderProgram.SetMat4("projection", voxelProj);
        glBindVertexArray(this->particleVAO);
        glDrawArraysInstanced(
            GL_TRIANGLE_FAN, 0, 4, 
            static_cast<GLsizei>(chunkMatrix.particles.size())
        );
    }

    if (this->debugRendering) {
        glm::vec4 green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        glm::vec4 red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        for (auto& chunk : chunkMatrix.Grid) {
            // draw chunk AABBs
            if(chunk->GetAABB().Overlaps(player->Camera)) {
                glm::vec2 start = {
                    chunk->GetAABB().corner.getX(),
                    chunk->GetAABB().corner.getY()
                };
                glm::vec2 end = {
                    chunk->GetAABB().corner.getX() + chunk->GetAABB().size.getX(),
                    chunk->GetAABB().corner.getY() + chunk->GetAABB().size.getY()
                };

                std::vector<glm::vec2> points = {
                    start,
                    {end.x, start.y},
                    end,
                    {start.x, end.y}
                };
                this->DrawClosedShape(points, red, voxelProj, 2.0f);

                fontRenderer.RenderText(
                    std::to_string(chunk->GetPos().getX()) + 
                        ", " + 
                        std::to_string(chunk->GetPos().getY()),
                    fontRenderer.pixelFont,
                    Vec2f(chunk->GetAABB().corner.getX()+2, chunk->GetAABB().corner.getY()+chunk->GetAABB().size.getY()-1),
                    1.0f,
                    glm::vec4(0.3f, 0.3f, 0.3f, 0.6f),
                    voxelProj
                );
                
                //draw dirty rects
                if(chunk->dirtyRect.IsEmpty()) continue;

                glm::vec2 dirtyStart = {
                    chunk->dirtyRect.start.getX() + chunk->GetAABB().corner.getX(),
                    chunk->dirtyRect.start.getY() + chunk->GetAABB().corner.getY()
                };
                glm::vec2 dirtyEnd = {
                    chunk->dirtyRect.end.getX() - chunk->dirtyRect.start.getX() + dirtyStart.x,
                    chunk->dirtyRect.end.getY() - chunk->dirtyRect.start.getY() + dirtyStart.y
                };

                std::vector<glm::vec2> dirtyRectPoints = {
                    dirtyStart,
                    {dirtyEnd.x, dirtyStart.y},
                    dirtyEnd,
                    {dirtyStart.x, dirtyEnd.y}
                };
                this->DrawClosedShape(dirtyRectPoints, green, voxelProj, 1.0f);
            }
        }

        // show voxel name, temperature and amount at mouse position
        Volume::VoxelElement* voxelAtMousePos = chunkMatrix.VirtualGetAt(
            Vec2i(
                static_cast<int>(mousePos.getX()/Volume::Chunk::RENDER_VOXEL_SIZE + player->Camera.corner.getX()),
                static_cast<int>(mousePos.getY()/Volume::Chunk::RENDER_VOXEL_SIZE + player->Camera.corner.getY())
            )
        );
        glm::vec4 fontColor = glm::vec4(0.1f, 0.1f, 0.1f, 0.6f);
        if(voxelAtMousePos != nullptr){
            fontRenderer.RenderText(
                "Voxel: " + voxelAtMousePos->properties->name,
                fontRenderer.pixelFont,
                Vec2f(5, 10),
                1.0f,
                fontColor,
                screenProj
            );
            fontRenderer.RenderText(
                "Temperature: " + std::to_string(static_cast<int>(voxelAtMousePos->temperature.GetCelsius())) + "C",
                fontRenderer.pixelFont,
                Vec2f(5, 20),
                1.0f,
                fontColor,
                screenProj
            );
            fontRenderer.RenderText(
                "Amount: " + std::to_string(static_cast<int>(voxelAtMousePos->amount)),
                fontRenderer.pixelFont,
                Vec2f(5, 30),
                1.0f,
                fontColor,
                screenProj
            );
        }
    }

    this->RenderIMGUI(chunkMatrix);

    SDL_GL_SwapWindow(r_window);
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "GL error: " << err << std::endl;
    }
}

void GameRenderer::DrawClosedShape(const std::vector<glm::vec2> &points, const glm::vec4 &color, 
    glm::mat4 projection, float lineWidth)
{
    if(points.size() < 3) {
        std::cerr << "Error: Cannot draw closed shape with less than 3 points." << std::endl;
        return;
    }

    closedShapeRenderProgram.Use();
    closedShapeRenderProgram.SetVec4("uColor", color);
    closedShapeRenderProgram.SetMat4("uProjection", projection);

    glBindBuffer(GL_ARRAY_BUFFER, closedShapeVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(glm::vec2), points.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(closedShapeVAO);

    glLineWidth(lineWidth);

    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(points.size()));
}

void GameRenderer::DrawClosedShape(const GLuint VAO, const GLsizei size, const glm::vec4 &color, 
    glm::mat4 projection, float lineWidth)
{
    if(size < 3) {
        std::cerr << "Error: Cannot draw closed shape with less than 3 points." << std::endl;
        return;
    }

    closedShapeRenderProgram.Use();
    closedShapeRenderProgram.SetVec4("uColor", color);
    closedShapeRenderProgram.SetMat4("uProjection", projection);

    glBindVertexArray(VAO);

    glLineWidth(lineWidth);

    glDrawArrays(GL_LINE_LOOP, 0, size);
}

void GameRenderer::RenderIMGUI(ChunkMatrix &chunkMatrix)
{
    Game::Player *player = GameEngine::instance->Player;

    ImGui_ImplSDL2_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("VoxaEngine Debug Panel");

    ImGui::Text("FPS: %lf", GameEngine::instance->FPS);
    ImGui::Text("LAST %d FPS AVG: %lf", AVG_FPS_SIZE_COUNT, GameEngine::instance->avgFPS);
    
    const char* voxelTypeNames[] = {
        "Dirt", "Grass", "Stone", "Sand", "Oxygen",
        "Water", "Fire", "Plasma", "Carbon_Dioxide", "Iron", "Rust", "Wood", "Empty",
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
    ImGui::DragInt("Placement Amount", &GameEngine::instance->placeVoxelAmount, 10, 1, 2000);
    ImGui::Checkbox("Place Unmovable Solid Voxels", &GameEngine::instance->placeUnmovableSolidVoxels);
    if(ImGui::Button("Toggle Debug Rendering")) ToggleDebugRendering();
    ImGui::Checkbox("Show Heat Around Cursor", &showHeatAroundCursor);

    ImGui::Checkbox("No Clip", &player->NoClip);
    ImGui::Checkbox("Heat Simulation", &GameEngine::instance->runHeatSimulation);
    ImGui::Checkbox("Pressure Simulation", &GameEngine::instance->runPressureSimulation);
    ImGui::DragFloat("Fixed Update speed", &GameEngine::instance->fixedDeltaTime, 0.05f, 1/30.0, 4);
    ImGui::DragFloat("Simulation Update speed", &GameEngine::instance->simulationFixedDeltaTime, 0.05f, 1/30.0, 4);

    ImGui::Text("Loaded chunks: %lld", chunkMatrix.Grid.size());

    ImGui::Checkbox("Player Gun", &player->gunEnabled);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GameRenderer::ToggleDebugRendering()
{
    this->debugRendering = !this->debugRendering;
}

void GameRenderer::UpdateParticleVBO(ChunkMatrix &chunkMatrix)
{
    std::vector<Particle::ParticleRenderData> particleData;
    for (Particle::VoxelParticle* particle : chunkMatrix.particles) {
        particleData.push_back({
            .position = glm::vec2(particle->GetPosition().getX(), particle->GetPosition().getY()),
            .color = particle->color.getGLMVec4()
        });
    }

    glBindBuffer(GL_ARRAY_BUFFER, this->particleVBO);
    glBufferData(GL_ARRAY_BUFFER, particleData.size() * sizeof(Particle::ParticleRenderData), particleData.data(), GL_DYNAMIC_DRAW);
}
