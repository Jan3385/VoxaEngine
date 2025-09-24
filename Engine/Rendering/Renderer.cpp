#include "Renderer.h"

#include "GameEngine.h"

#include <iostream>
#include <vector>
#include <mutex>
#include <math.h>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "World/ChunkMatrix.h"
#include "Math/AABB.h"

#include "World/Particle.h"

GameRenderer::GameRenderer()
{
    
}

GameRenderer::GameRenderer(SDL_GLContext *glContext, RGB backgroundColor, bool loadChunksInView)
    : loadChunksInView(loadChunksInView), backgroundColor(backgroundColor)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        std::cerr << "Error initializing SDL2: " << SDL_GetError() << std::endl;
    }

    IMGUI_CHECKVERSION();

    this->Camera = AABB(
        Vec2f((800.0/Volume::Chunk::RENDER_VOXEL_SIZE)/2, (600.0/Volume::Chunk::RENDER_VOXEL_SIZE)/2), 
        Vec2f(800.0/Volume::Chunk::RENDER_VOXEL_SIZE, 600.0/Volume::Chunk::RENDER_VOXEL_SIZE)
    );

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

    // Enables Alpha Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disables Depth Testing
    glDisable(GL_DEPTH_TEST);

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

    std::cout << "Compiling voxel render shaders... ";
    this->voxelRenderProgram = new Shader::RenderShader(
        "VoxelArrayRender"
    );
    this->temperatureRenderProgram = new Shader::RenderShader(
        "TemperatureVoxelArray"
    );
    this->particleRenderProgram = new Shader::RenderShader(
        "ParticleArrayRender"
    );
    this->closedShapeRenderProgram = new Shader::RenderShader(
        "ClosedShape"
    );
    this->cursorRenderProgram = new Shader::RenderShader(
        "CursorRender"
    );
    std::cout << "[Done]" << std::endl;

    // Quad VBO setup ----
    const glm::vec2 quad[] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };
    quadBuffer = new Shader::GLBuffer<glm::vec2, GL_ARRAY_BUFFER>("Quad VBO");
    quadBuffer->SetData(quad, 4, GL_STATIC_DRAW);
    // --------------------

    particleVBO = Shader::GLBuffer<Particle::ParticleRenderData, GL_ARRAY_BUFFER>("Particle Render VBO");

    //Particle VAO setup ----
    particleVAO = Shader::GLVertexArray("Particle Render VAO");
    particleVAO.Bind();

    particleVAO.AddAttribute<glm::vec2>(0, 2, *quadBuffer, GL_FALSE, 0, 0); // location 0: vec2 texCoord

    particleVAO.AddAttribute<glm::vec2>(1, 2, particleVBO, GL_FALSE, offsetof(Particle::ParticleRenderData, position), 1); // location 1: vec2 worldPos
    particleVAO.AddAttribute<glm::vec4>(2, 4, particleVBO, GL_FALSE, offsetof(Particle::ParticleRenderData, color), 1);     // location 2: vec4 color
    
    particleVAO.Unbind();
    // --------------

    // Closed shape VAO setup ----
    closedShapeVAO = Shader::GLVertexArray("Closed Shape VAO");
    closedShapeVBO = Shader::GLBuffer<glm::vec2, GL_ARRAY_BUFFER>("Closed Shape VBO");

    closedShapeVAO.Bind();
    closedShapeVAO.AddAttribute<glm::vec2>(0, 2, closedShapeVBO, GL_FALSE, 0, 0); // location 0: vec2 vertex

    closedShapeVAO.Unbind();
    // --------------

    // Cursor VAO setup ----
    cursorVAO = Shader::GLVertexArray("Cursor VAO");
    cursorVAO.Bind();

    cursorVAO.AddAttribute<glm::vec2>(0, 2, *quadBuffer, GL_FALSE, 0, 0);
    // --------------

    this->fontRenderer.Initialize();
    this->spriteRenderer.Initialize();
}

GameRenderer::~GameRenderer()
{
    if(this->voxelRenderProgram)
        delete this->voxelRenderProgram;
    if(this->temperatureRenderProgram)
        delete this->temperatureRenderProgram;
    if(this->particleRenderProgram)
        delete this->particleRenderProgram;
    if(this->closedShapeRenderProgram)
        delete this->closedShapeRenderProgram;
    if(this->cursorRenderProgram)
        delete this->cursorRenderProgram;

    delete quadBuffer;

    ImGui_ImplSDL2_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(r_window);
    
    SDL_Quit();
}

void GameRenderer::SetCameraPosition(Vec2f centerPos)
{
    static constexpr float halfChunk = Volume::Chunk::CHUNK_SIZE / 2.0f;
    Vec2f prevPosition = this->Camera.corner;

    this->Camera.corner.x = centerPos.x - this->Camera.size.x / 2.0f;
    this->Camera.corner.y = centerPos.y - this->Camera.size.y / 2.0f;

    // Camera position changed
    if(prevPosition != this->Camera.corner && this->loadChunksInView) {
        // Generate any new chunks
        Vec2f cameraMin = Camera.corner;
        Vec2f cameraMax = Camera.corner + Camera.size;

        Vec2f adjustedMin = cameraMin - Vec2f(halfChunk, halfChunk);
        Vec2f adjustedMax = cameraMax + Vec2f(halfChunk, halfChunk);

        Vec2i chunkMin = ChunkMatrix::WorldToChunkPosition(adjustedMin);
        Vec2i chunkMax = ChunkMatrix::WorldToChunkPosition(adjustedMax);

        std::vector<Vec2i> chunksToLoad;
        for (int x = chunkMin.x; x <= chunkMax.x; ++x) {
            for (int y = chunkMin.y; y <= chunkMax.y; ++y) {
                if(!GameEngine::instance->GetActiveChunkMatrix()->IsValidChunkPosition(Vec2i(x, y))) continue;    // Skip invalid chunk positions
                if(GameEngine::instance->GetActiveChunkMatrix()->GetChunkAtChunkPosition(Vec2i(x, y))) continue;  // If the chunk already exists, skip it
                chunksToLoad.push_back(Vec2i(x, y));
            }
        }

        for (const auto& chunkPos : chunksToLoad) {
            GameEngine::instance->GetActiveChunkMatrix()->GenerateChunk(chunkPos);
        }
    }
}

/// @brief Set when the window is resized
/// @param size size in voxels
void GameRenderer::SetCameraSize(Vec2f size)
{
    this->Camera.size = size;
}

void GameRenderer::Render(ChunkMatrix &chunkMatrix, Vec2i mousePos, IGame *game)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[PreRender] GL error: [" << err << "]" << std::endl;
    }

    // clear screen with a light blue color
    glClearColor(
        this->backgroundColor.r / 255.0f,
        this->backgroundColor.g / 255.0f,
        this->backgroundColor.b / 255.0f,
        1.0f
    );
    glClear(GL_COLOR_BUFFER_BIT);


    // set up projections
    glm::mat4 voxelProj = glm::ortho(
        this->Camera.corner.x, this->Camera.corner.x + this->Camera.size.x, 
        this->Camera.corner.y + this->Camera.size.y, this->Camera.corner.y,
        -1.0f, 1.0f
    );
    glm::mat4 screenProj = glm::ortho(
        0.0f, this->Camera.size.x, 
        this->Camera.size.y, 0.0f,
        -1.0f, 1.0f
    );

    this->RenderVoxelObjects(chunkMatrix, voxelProj);

    VoxelObject *player = GameEngine::instance->GetPlayer();
    if(player)
        this->RenderPlayer(player, voxelProj);

    this->RenderChunks(chunkMatrix, voxelProj);

    //glm::vec2 mousePosInWorld = {
    //    mousePos.x / Volume::Chunk::RENDER_VOXEL_SIZE + static_cast<int>(this->Camera.corner.x),
    //    mousePos.y / Volume::Chunk::RENDER_VOXEL_SIZE + static_cast<int>(this->Camera.corner.y)
    //};
    Vec2f mousePosInWorldF = ChunkMatrix::MousePosToWorldPos(mousePos, this->Camera.corner);
    glm::vec2 mousePosInWorldInt = glm::vec2(static_cast<int>(mousePosInWorldF.x), static_cast<int>(mousePosInWorldF.y));

    this->RenderHeat(chunkMatrix, mousePosInWorldInt, voxelProj);

    this->RenderParticles(chunkMatrix, voxelProj);

    if (this->debugRendering)
        this->RenderDebugMode(chunkMatrix, mousePosInWorldInt, voxelProj, screenProj);

    if(this->renderMeshData)
        this->RenderMeshData(chunkMatrix, voxelProj);


    // prepare IMGUI for the game renderer
    ImGui_ImplSDL2_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    game->Render(voxelProj, screenProj);

    // Render the ImGui frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(r_window);

    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[Render] GL error: [" << err << "]" << std::endl;
    }
}

void GameRenderer::DrawClosedShape(const std::vector<glm::vec2> &points, const glm::vec4 &color, 
    glm::mat4 projection, float lineWidth)
{
    if(points.size() < 3) {
        std::cerr << "Error: Cannot draw closed shape with less than 3 points." << std::endl;
        return;
    }

    closedShapeRenderProgram->Use();
    closedShapeRenderProgram->SetVec4("uColor", color);
    closedShapeRenderProgram->SetMat4("uProjection", projection);

    closedShapeVBO.SetData(points, GL_DYNAMIC_DRAW);

    closedShapeVAO.Bind();

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

    closedShapeRenderProgram->Use();
    closedShapeRenderProgram->SetVec4("uColor", color);
    closedShapeRenderProgram->SetMat4("uProjection", projection);

    glBindVertexArray(VAO);

    glLineWidth(lineWidth);

    glDrawArrays(GL_LINE_LOOP, 0, size);
}

void GameRenderer::ToggleDebugRendering()
{
    this->debugRendering = !this->debugRendering;
}

void GameRenderer::RenderVoxelObjects(ChunkMatrix &chunkMatrix, glm::mat4 projection)
{
    this->voxelRenderProgram->Use();
    this->voxelRenderProgram->SetMat4("projection", projection);

    for (VoxelObject* object : chunkMatrix.voxelObjects) {
        if(object == nullptr) continue;
        if(!object->ShouldRender()) continue;

        auto pos = object->GetPosition();

        unsigned int voxelCount = object->UpdateRenderBuffer();

        object->renderVoxelArray.Bind();
        glDrawArraysInstanced(
            GL_TRIANGLE_FAN, 0, 4, 
            static_cast<GLsizei>(voxelCount)
        );
    }
}

void GameRenderer::RenderPlayer(VoxelObject *player, glm::mat4 projection)
{
    this->voxelRenderProgram->Use();
    this->voxelRenderProgram->SetMat4("projection", projection);

    if(player->ShouldRender()) {
        unsigned int voxelCount = player->UpdateRenderBuffer();

        player->renderVoxelArray.Bind();
        glDrawArraysInstanced(
            GL_TRIANGLE_FAN, 0, 4, 
            static_cast<GLsizei>(voxelCount)
        );
    }
}

void GameRenderer::RenderChunks(ChunkMatrix &chunkMatrix,  glm::mat4 projection)
{
    this->voxelRenderProgram->Use();
    this->voxelRenderProgram->SetMat4("projection", projection);

    for (auto& chunk : chunkMatrix.Grid) {
        if(!chunk->IsInitialized()) continue;
        
        if(chunk->GetAABB().Overlaps(this->Camera)){
            chunk->Render(false);

            chunk->renderVoxelVAO.Bind();
            glDrawArraysInstanced(
                GL_TRIANGLE_FAN, 0, 4, 
                Volume::Chunk::CHUNK_SIZE_SQUARED
            );
        }
    }
}

void GameRenderer::RenderHeat(ChunkMatrix &chunkMatrix, glm::vec2 mousePos,  glm::mat4 projection)
{
    this->temperatureRenderProgram->Use();
    this->temperatureRenderProgram->SetMat4("projection", projection);
    this->temperatureRenderProgram->SetBool("showHeatAroundCursor", this->showHeatAroundCursor);
    this->temperatureRenderProgram->SetVec2("cursorPosition", mousePos);

    for (auto& chunk : chunkMatrix.Grid) {
        if(chunk->GetAABB().Overlaps(this->Camera)) {
            chunk->heatRenderingVAO.Bind();
            glDrawArraysInstanced(
                GL_TRIANGLE_FAN, 0, 4, 
                Volume::Chunk::CHUNK_SIZE_SQUARED
            );
        }
    }
}

void GameRenderer::RenderParticles(ChunkMatrix &chunkMatrix, glm::mat4 projection)
{
    if(chunkMatrix.particles.size() > 0){
        this->UpdateParticleVBO(chunkMatrix);
        this->particleRenderProgram->Use();
        this->particleRenderProgram->SetMat4("projection", projection);
        
        particleVAO.Bind();
        glDrawArraysInstanced(
            GL_TRIANGLE_FAN, 0, 4, 
            static_cast<GLsizei>(chunkMatrix.particles.size())
        );
    }
}

void GameRenderer::SetVSYNC(bool enabled)
{
    if(!enabled)
        SDL_GL_SetSwapInterval(0);
    else{
        bool failed = SDL_GL_SetSwapInterval(-1) == -1;
        
        if(!failed)
            SDL_GL_SetSwapInterval(1);
    }
}

void GameRenderer::RenderCursor(glm::vec2 mousePos, glm::mat4 projection, int cursorSize)
{
    this->cursorRenderProgram->Use();
    this->cursorRenderProgram->SetVec2("size", glm::vec2(cursorSize, cursorSize));
    this->cursorRenderProgram->SetVec2("cursorPosition", mousePos);
    this->cursorRenderProgram->SetMat4("projection", projection);

    this->cursorRenderProgram->SetVec4("outlineColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));

    cursorVAO.Bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void GameRenderer::RenderDebugMode(ChunkMatrix &chunkMatrix, glm::vec2 mousePos, glm::mat4 voxelProj, glm::mat4 screenProj)
{
    glm::vec4 green = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 red = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    for (auto& chunk : chunkMatrix.Grid) {
        // draw chunk AABBs
        if(chunk->GetAABB().Overlaps(this->Camera)) {
            glm::vec2 start = {
                chunk->GetAABB().corner.x,
                chunk->GetAABB().corner.y
            };
            glm::vec2 end = {
                chunk->GetAABB().corner.x + chunk->GetAABB().size.x,
                chunk->GetAABB().corner.y + chunk->GetAABB().size.y
            };

            std::vector<glm::vec2> points = {
                start,
                {end.x, start.y},
                end,
                {start.x, end.y}
            };
            this->DrawClosedShape(points, red, voxelProj, 2.0f);

            fontRenderer.RenderText(
                std::to_string(chunk->GetPos().x) + 
                    ", " + 
                    std::to_string(chunk->GetPos().y),
                fontRenderer.pixelFont,
                Vec2f(chunk->GetAABB().corner.x+2, chunk->GetAABB().corner.y+chunk->GetAABB().size.y-1),
                1.0f,
                glm::vec4(0.3f, 0.3f, 0.3f, 0.6f),
                voxelProj
            );
            
            //draw dirty rects
            if(chunk->dirtyRect.IsEmpty()) continue;

            glm::vec2 dirtyStart = {
                chunk->dirtyRect.start.x + chunk->GetAABB().corner.x,
                chunk->dirtyRect.start.y + chunk->GetAABB().corner.y
            };
            glm::vec2 dirtyEnd = {
                chunk->dirtyRect.end.x - chunk->dirtyRect.start.x + dirtyStart.x,
                chunk->dirtyRect.end.y - chunk->dirtyRect.start.y + dirtyStart.y
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
        Vec2i(mousePos.x, mousePos.y),
        true
    );
    Volume::Chunk* chunkAtMousePos = chunkMatrix.GetChunkAtWorldPosition(
        Vec2i(mousePos.x, mousePos.y)
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
            "State: " + std::to_string(static_cast<int>(voxelAtMousePos->GetState())),
            fontRenderer.pixelFont,
            Vec2f(5, 20),
            1.0f,
            fontColor,
            screenProj
        );
        fontRenderer.RenderText(
            "Temperature: " + std::to_string(static_cast<int>(voxelAtMousePos->temperature.GetCelsius())) + "C",
            fontRenderer.pixelFont,
            Vec2f(5, 30),
            1.0f,
            fontColor,
            screenProj
        );
        fontRenderer.RenderText(
            "Amount: " + std::to_string(static_cast<int>(voxelAtMousePos->amount)),
            fontRenderer.pixelFont,
            Vec2f(5, 40),
            1.0f,
            fontColor,
            screenProj
        );
        fontRenderer.RenderText(
            "Chunk Ticket: " + std::to_string(static_cast<int>(chunkAtMousePos->bufferTicket)),
            fontRenderer.pixelFont,
            Vec2f(5, 50),
            1.0f,
            fontColor,
            screenProj
        );
        fontRenderer.RenderText(
            "Chunk Index: " + std::to_string(chunkMatrix.Grid.size() > 0 ? std::distance(chunkMatrix.Grid.begin(), std::find(chunkMatrix.Grid.begin(), chunkMatrix.Grid.end(), chunkAtMousePos)) : -1),
            fontRenderer.pixelFont,
            Vec2f(5, 60),
            1.0f,
            fontColor,
            screenProj
        );
    }
}

void GameRenderer::RenderMeshData(ChunkMatrix &chunkMatrix, glm::mat4 projection)
{
    // Draw chunk mesh data as before
    for (auto& chunk : chunkMatrix.Grid) {
        if(chunk->GetAABB().Overlaps(this->Camera)) {
            for (const Triangle& t : chunk->GetColliders()) {
                std::vector<glm::vec2> points = {
                    glm::vec2(t.a.x, t.a.y) + glm::vec2(chunk->GetAABB().corner.x, chunk->GetAABB().corner.y),
                    glm::vec2(t.b.x, t.b.y) + glm::vec2(chunk->GetAABB().corner.x, chunk->GetAABB().corner.y),
                    glm::vec2(t.c.x, t.c.y) + glm::vec2(chunk->GetAABB().corner.x, chunk->GetAABB().corner.y)
                };
                this->DrawClosedShape(
                    points, 
                    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 
                    projection, 
                    1.0f
                );
            }
            int numOfSeparatingVectors = std::count_if(
                chunk->GetEdges().begin(), chunk->GetEdges().end(),
                [](const b2Vec2& edge) { return edge.x == -1 && edge.y == -1; }
            );
            std::vector<std::vector<glm::vec2>> edges(numOfSeparatingVectors); // reserve space for edges
            int i = 0;
            for(const b2Vec2& edge : chunk->GetEdges()) {
                if(edge.x == -1 && edge.y == -1) { // (-1, -1) is a separating vector
                    ++i;
                    continue;
                }
                glm::vec2 worldSpaceEdge = glm::vec2(edge.x, edge.y) + glm::vec2(chunk->GetAABB().corner.x, chunk->GetAABB().corner.y);
                edges[i].push_back(worldSpaceEdge);
            }
            for(auto& polygon : edges) {
                if(polygon.size() < 3) continue; // skip edges with less than 3 points
                this->DrawClosedShape(
                    polygon, 
                    glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 
                    projection, 
                    2.0f
                );
            }
        }
    }

    for (PhysicsObject* physObj : GameEngine::instance->GetActiveChunkMatrix()->physicsObjects) {
        float rot = physObj->GetRotation();
        float cosR = std::cos(rot);
        float sinR = std::sin(rot);
        Vec2f pos = physObj->GetPosition();
        float cx = physObj->GetSize().x / 2.0f;
        float cy = physObj->GetSize().y / 2.0f;

        for (const Triangle& t : physObj->triangleColliders) {
            std::vector<glm::vec2> tri(3);
            // Rotate and translate each vertex, centering mesh
            for (int i = 0; i < 3; ++i) {
                const b2Vec2* v = (i == 0) ? &t.a : (i == 1) ? &t.b : &t.c;
                float x = v->x - cx;
                float y = v->y - cy;
                float xr = cosR * x - sinR * y + pos.x;
                float yr = sinR * x + cosR * y + pos.y;
                tri[i] = glm::vec2(xr, yr);
            }
            this->DrawClosedShape(tri, glm::vec4(0.2f, 0.2f, 1.0f, 1.0f), projection, 2.0f);
        }
        glm::vec2 aabbStart = {
            physObj->GetBoundingBox().corner.x,
            physObj->GetBoundingBox().corner.y
        };
        glm::vec2 aabbEnd = {
            physObj->GetBoundingBox().corner.x + physObj->GetBoundingBox().size.x,
            physObj->GetBoundingBox().corner.y + physObj->GetBoundingBox().size.y
        };
        std::vector<glm::vec2> aabbPoints = {
            aabbStart,
            {aabbEnd.x, aabbStart.y},
            aabbEnd,
            {aabbStart.x, aabbEnd.y}
        };
        this->DrawClosedShape(aabbPoints, glm::vec4(0.8f, 0.2f, 0.8f, 1.0f), projection, 1.5f);
    }
}

void GameRenderer::UpdateParticleVBO(ChunkMatrix &chunkMatrix)
{
    std::vector<Particle::ParticleRenderData> particleData;
    for (Particle::VoxelParticle* particle : chunkMatrix.particles) {
        particleData.push_back({
            .position = glm::vec2(particle->GetPosition().x, particle->GetPosition().y),
            .color = particle->color.getGLMVec4()
        });
    }

    particleVBO.SetData(particleData, GL_DYNAMIC_DRAW);
    particleVAO.Bind();
}
