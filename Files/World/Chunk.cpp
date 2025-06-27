#include "World/Chunk.h"
#include "World/Voxel.h"
#include "World/VoxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>
#include "GameEngine.h"
#include "World/Particles/SolidFallingParticle.h"
#include "World/ParticleGenerators/LaserParticleGenerator.h"
#include "Chunk.h"


using namespace Volume; 

void DirtyRect::Include(Vec2i pos)
{
    m_startW.x(std::min(m_startW.getX(), pos.getX()));
    m_startW.y(std::min(m_startW.getY(), pos.getY()));
    m_endW.x(std::max(m_endW.getX(), pos.getX()));
    m_endW.y(std::max(m_endW.getY(), pos.getY()));
}

void DirtyRect::Update()
{
    this->start = m_startW-Vec2i(DIRTY_RECT_PADDING, DIRTY_RECT_PADDING);  
    this->end = m_endW+Vec2i(DIRTY_RECT_PADDING, DIRTY_RECT_PADDING);

    m_startW = Vec2i(INT_MAX, INT_MAX);
    m_endW = Vec2i(INT_MIN, INT_MIN);
}

bool DirtyRect::IsEmpty() const
{
    return this->start.getX() == INT_MAX-DIRTY_RECT_PADDING;
}

Volume::Chunk::Chunk(const Vec2i &pos) : m_x(pos.getX()), m_y(pos.getY())
{
    Vec2i chunkWorldPos = Vec2i(m_x * CHUNK_SIZE, m_y * CHUNK_SIZE);
    for(uint8_t x = 0; x < CHUNK_SIZE; x++){
        for(uint8_t y = 0; y < CHUNK_SIZE; y++){
            renderData[x][y].position = glm::ivec2(
                (chunkWorldPos.getX() + x), //* RENDER_VOXEL_SIZE,
                (chunkWorldPos.getY() + y) //* RENDER_VOXEL_SIZE
            );
            renderData[x][y].color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // default color white
        }
        this->UpdateRenderBufferRanges[x] = Math::Range(0, CHUNK_SIZE - 1);
    }
}

Volume::Chunk::~Chunk()
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(voxels.size()); i++)
    {
        for (uint8_t j = 0; j < static_cast<uint8_t>(voxels[i].size()); j++)
        {
            delete voxels[i][j];
        }
    }

    // Delete OpenGL resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &instanceVBO);
    VAO = 0;
    instanceVBO = 0;
}
void Volume::Chunk::SetVBOData()
{
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ChunkVoxelRenderData) * CHUNK_SIZE_SQUARED, nullptr, GL_DYNAMIC_DRAW);

    // VAO setup ----
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, GameEngine::instance->renderer->quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glVertexAttribIPointer(1, 2, GL_INT, sizeof(ChunkVoxelRenderData), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // instance attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ChunkVoxelRenderData), (void*)(sizeof(glm::vec2)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    // --------------

    // Update the instance VBO with the new render data
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ChunkVoxelRenderData) * CHUNK_SIZE_SQUARED, renderData);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
bool Volume::Chunk::ShouldChunkDelete(AABB &Camera) const
{
    if(lastCheckedCountDown > 0) return false;
    if(!this->dirtyRect.IsEmpty()) return false;

    return true;
}
bool Volume::Chunk::ShouldChunkCalculateHeat() const
{
    return forceHeatUpdate;
}
bool Volume::Chunk::ShouldChunkCalculatePressure() const
{
    return true;
    //TODO: fix when the vibe is right
    //return forcePressureUpdate;
}
void Volume::Chunk::UpdateVoxels(ChunkMatrix *matrix)
{
    for (int x = dirtyRect.end.getX(); x >= dirtyRect.start.getX(); --x)
    {
        //for (int y = dirtyRect.end.getY(); y >= dirtyRect.start.getY(); --y) -> better gasses, worse solids
        for (int y = dirtyRect.start.getY(); y <= dirtyRect.end.getY(); ++y) //better solids, worse gasses
        {
            if(x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) continue;
    		if (voxels[x][y]->Step(matrix)) {
                //add to dirty rect
                dirtyRect.Include(Vec2i(x, y));

                //if voxel is at the edge of chunk, update neighbour chunk
    			if (x == 0) { // left
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x - 1, m_y));
                    if(c)c->dirtyRect.Include(Vec2i(CHUNK_SIZE - 1, y));
    			}
    			else if (x == CHUNK_SIZE - 1) { // right
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x + 1, m_y));
                    if(c)c->dirtyRect.Include(Vec2i(0, y));
    			}
    			if (y == 0) { // top
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y - 1));
                    if(c)c->dirtyRect.Include(Vec2i(x, CHUNK_SIZE - 1));
    			}
    			else if (y == CHUNK_SIZE - 1) { // bottom
                    Chunk* c = matrix->GetChunkAtChunkPosition(Vec2i(m_x, m_y + 1));
                    if(c)c->dirtyRect.Include(Vec2i(x, 0));
    			}
    		}

        }
    }
}

/**
 * Fills the provided buffers with data for compute shaders.
 * provides flattened arrays of voxel data for shaders
 */
void Volume::Chunk::GetShadersData(
    float temperatureBuffer[], 
    float heatCapacityBuffer[], 
    float heatConductivityBuffer[], 
    float pressureBuffer[], 
    uint32_t idBuffer[], 
    int chunkNumber) const
{
    #pragma omp parallel for collapse(2)
    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            int index = chunkNumber * Chunk::CHUNK_SIZE_SQUARED + y * Chunk::CHUNK_SIZE + x;

            Volume::VoxelElement* voxel = this->voxels[x][y];
            const Volume::VoxelProperty* props = voxel->properties;

            temperatureBuffer[index] = voxel->temperature.GetCelsius();
            heatCapacityBuffer[index] = props->HeatCapacity;
            heatConductivityBuffer[index] = props->HeatConductivity;
            pressureBuffer[index] = voxel->amount;
            idBuffer[index] = props->id |
                (static_cast<uint32_t>(voxel->GetState() != State::Gas) << 31) |
                (static_cast<uint32_t>(voxel->GetState() == State::Liquid) << 30);
        }
    }
}

// resets voxel update data, DO NOT CALL WITHOUT LOCKING THE VOXELMUTEX, main use for simulation thread
void Volume::Chunk::SIM_ResetVoxelUpdateData()
{
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < CHUNK_SIZE; ++y)
    {
        for (int x = 0; x < CHUNK_SIZE; ++x)
        {
            voxels[x][y]->updatedThisFrame = false;
        }
    }
}

void Volume::Chunk::Render(bool debugRender)
{
    for(uint8_t x = 0; x < CHUNK_SIZE; ++x) {
        if(this->UpdateRenderBufferRanges[x].IsEmpty()) continue;
        for(uint8_t y = this->UpdateRenderBufferRanges[x].Start(); y <= this->UpdateRenderBufferRanges[x].End(); ++y) {
            renderData[x][y].color = voxels[x][y]->color.getGLMVec4();
        }
    }

    // Update the instance VBO with the new render data
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    for(uint8_t x = 0; x < CHUNK_SIZE; ++x) {
        if(this->UpdateRenderBufferRanges[x].IsEmpty()) continue;

        if(this->UpdateRenderBufferRanges[x].Start() < 0 || 
           this->UpdateRenderBufferRanges[x].End() >= CHUNK_SIZE) {
            std::cerr << "Chunk " << m_x << ", " << m_y << " has invalid render range: "
                      << this->UpdateRenderBufferRanges[x].Start() << " - "
                      << this->UpdateRenderBufferRanges[x].End() << std::endl;
            continue;
        }

        glBufferSubData(GL_ARRAY_BUFFER,
            sizeof(ChunkVoxelRenderData) * (CHUNK_SIZE * x + this->UpdateRenderBufferRanges[x].Start()), 
            sizeof(ChunkVoxelRenderData) * (this->UpdateRenderBufferRanges[x].End() - this->UpdateRenderBufferRanges[x].Start() + 1), 
            &renderData[x][this->UpdateRenderBufferRanges[x].Start()]
        );

        this->UpdateRenderBufferRanges[x].Reset();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
Vec2i Volume::Chunk::GetPos() const
{
    return Vec2i(m_x, m_y);
}
AABB Volume::Chunk::GetAABB() const
{
    return AABB(
        Vec2f(m_x * CHUNK_SIZE, m_y * CHUNK_SIZE), 
        Vec2f(CHUNK_SIZE, CHUNK_SIZE));
}

