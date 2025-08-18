#include "Chunk.h"
#include "World/Voxel.h"
#include "World/voxelTypes.h"	
#include <math.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>

#include "GameEngine.h"
#include "World/Particles/SolidFallingParticle.h"
#include "World/ParticleGenerators/LaserParticleGenerator.h"
#include "Physics/Physics.h"


using namespace Volume; 

void DirtyRect::Include(Vec2i pos)
{
    m_startW.x = std::min(m_startW.x, pos.x);
    m_startW.y = std::min(m_startW.y, pos.y);
    m_endW.x = std::max(m_endW.x, pos.x);
    m_endW.y = std::max(m_endW.y, pos.y);
}

void DirtyRect::Update()
{
    this->start = m_startW-Vec2i(DIRTY_RECT_PADDING, DIRTY_RECT_PADDING);  
    this->end = m_endW+Vec2i(DIRTY_RECT_PADDING*2, DIRTY_RECT_PADDING*2);

    this->start.x = std::max(this->start.x, 0);
    this->start.y = std::max(this->start.y, 0);
    this->end.x = std::min(this->end.x, static_cast<int>(Chunk::CHUNK_SIZE));
    this->end.y = std::min(this->end.y, static_cast<int>(Chunk::CHUNK_SIZE));

    m_startW = Vec2i(INT_MAX, INT_MAX);
    m_endW = Vec2i(INT_MIN, INT_MIN);
}

bool DirtyRect::IsEmpty() const
{
    return this->start.x == INT_MAX-DIRTY_RECT_PADDING;
}

Volume::Chunk::Chunk(const Vec2i &pos) : m_x(pos.x), m_y(pos.y)
{
    Vec2i chunkWorldPos = Vec2i(m_x * CHUNK_SIZE, m_y * CHUNK_SIZE);
    for(uint8_t y = 0; y < CHUNK_SIZE; y++){
        for(uint8_t x = 0; x < CHUNK_SIZE; x++){
            renderData[y][x].position = glm::ivec2(
                (chunkWorldPos.x + x),  //* RENDER_VOXEL_SIZE,
                (chunkWorldPos.y + y)   //* RENDER_VOXEL_SIZE
            );
            renderData[y][x].color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // default color white
        }
        this->UpdateRenderBufferRanges[y] = Math::Range(0, CHUNK_SIZE - 1);
    }
}

Volume::Chunk::~Chunk()
{
    for (unsigned short int i = 0; i < Chunk::CHUNK_SIZE; i++)
    {
        for (unsigned short int  j = 0; j < Chunk::CHUNK_SIZE; j++)
        {
            delete voxels[i][j];
        }
    }

    // Delete OpenGL resources
    glDeleteVertexArrays(1, &renderVoxelVAO);
    glDeleteBuffers(1, &renderVBO);
    renderVoxelVAO = 0;
    renderVBO = 0;

    glDeleteVertexArrays(1, &heatRenderingVAO);
    glDeleteBuffers(1, &temperatureVBO);
    heatRenderingVAO = 0;
    temperatureVBO = 0;
}
void Volume::Chunk::SetVBOData()
{
    glGenBuffers(1, &renderVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VoxelRenderData) * CHUNK_SIZE_SQUARED, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &temperatureVBO);
    glBindBuffer(GL_ARRAY_BUFFER, temperatureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * CHUNK_SIZE_SQUARED, nullptr, GL_DYNAMIC_DRAW);

    // voxel rendering VAO setup ----
    glGenVertexArrays(1, &renderVoxelVAO);
    glBindVertexArray(renderVoxelVAO);

    glBindBuffer(GL_ARRAY_BUFFER, GameEngine::renderer->quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, renderVBO);
    glVertexAttribIPointer(1, 2, GL_INT, sizeof(VoxelRenderData), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // instance attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VoxelRenderData), (void*)(sizeof(glm::ivec2)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
    // --------------

    // heat rendering VAO setup ----
    glGenVertexArrays(1, &heatRenderingVAO);
    glBindVertexArray(heatRenderingVAO);

    glBindBuffer(GL_ARRAY_BUFFER, GameEngine::renderer->quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); // location 0
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, renderVBO);
    glVertexAttribIPointer(1, 2, GL_INT, sizeof(VoxelRenderData), (void*)0); // location 1
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VoxelRenderData), (void*)(sizeof(glm::vec2))); // location 2
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, temperatureVBO);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0); // location 3: heat
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
    // --------------

    // Update the instance VBO with the new render data
    glBindBuffer(GL_ARRAY_BUFFER, renderVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VoxelRenderData) * CHUNK_SIZE_SQUARED, renderData);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
bool Volume::Chunk::ShouldChunkDelete(AABB &Camera) const
{
    if(lastCheckedCountDown > 0) return false;
    if(!this->dirtyRect.IsEmpty()) return false;
    if(Camera.Expand(Chunk::CHUNK_SIZE/2).Overlaps(this->GetAABB())) return false;

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
    // update vector of objects in chunk
    this->voxelObjectInChunk.clear();
    for(VoxelObject* obj : matrix->voxelObjects)
    {
        if(obj->GetBoundingBox().Overlaps(this->GetAABB()))
        {
            this->voxelObjectInChunk.push_back(obj);
        }
    }

    if(dirtyRect.IsEmpty()) return;

    for (int x = dirtyRect.end.x; x >= dirtyRect.start.x; --x)
    {
        //for (int y = dirtyRect.end.y; y >= dirtyRect.start.y; --y) -> better gasses, worse solids
        for (int y = dirtyRect.start.y; y <= dirtyRect.end.y; ++y) //better solids, worse gasses
        {
            if(x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) continue;
    		if (voxels[y][x]->Step(matrix)) {
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

void Volume::Chunk::UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId)
{
    this->dirtyColliders = false;

    this->DestroyPhysicsBody();
    this->CreatePhysicsBody(worldId);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material = b2DefaultSurfaceMaterial();

    for(Triangle t : triangles){
        b2Hull hull;
        hull.points[0] = t.a;
        hull.points[1] = t.b;
        hull.points[2] = t.c;
        hull.count = 3;

        b2Polygon polygon = b2MakePolygon(
            &hull, 0.01f
        );

        b2CreatePolygonShape(
            m_physicsBody, &shapeDef, &polygon
        );
    }

    m_triangleColliders.assign(triangles.begin(), triangles.end());
    m_edges.assign(edges.begin(), edges.end());
}

/**
 * Fills the provided buffers with data for compute shaders.
 * provides flattened arrays of voxel data for shaders
 * Updates the internal temperature OpenGL buffer for the chunk
 */
void Volume::Chunk::GetShadersData(
    float temperatureBuffer[], 
    float heatCapacityBuffer[], 
    float heatConductivityBuffer[], 
    float pressureBuffer[], 
    uint32_t idBuffer[], 
    int chunkNumber) const
{
    #pragma omp parallel for
    for(int x = 0; x < Chunk::CHUNK_SIZE; ++x){
        for(int y = 0; y < Chunk::CHUNK_SIZE; ++y){
            int index = chunkNumber * Chunk::CHUNK_SIZE_SQUARED + y * Chunk::CHUNK_SIZE + x;

            Volume::VoxelElement* voxel = this->voxels[y][x];
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

void Volume::Chunk::UpdateInnerTemperatureBuffer()
{
    float temperatureVBOBuffer[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];

    #pragma omp parallel for collapse(2)
    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        for (int x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            temperatureVBOBuffer[y][x] = this->voxels[y][x]->temperature.GetCelsius();
        }
    }

    // Update the temperature VBO with the new data
    glBindBuffer(GL_ARRAY_BUFFER, temperatureVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * CHUNK_SIZE_SQUARED, temperatureVBOBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// resets voxel update data, DO NOT CALL WITHOUT LOCKING THE VOXELMUTEX, main use for simulation thread
void Volume::Chunk::SIM_ResetVoxelUpdateData()
{
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < CHUNK_SIZE; ++y)
    {
        for (int x = 0; x < CHUNK_SIZE; ++x)
        {
            voxels[y][x]->updatedThisFrame = false;
        }
    }
}

void Volume::Chunk::Render(bool debugRender)
{
    for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
        if(this->UpdateRenderBufferRanges[y].IsEmpty()) continue;
        for(uint8_t x = this->UpdateRenderBufferRanges[y].Start(); x <= this->UpdateRenderBufferRanges[y].End(); ++x) {
            renderData[y][x].color = voxels[y][x]->color.getGLMVec4();
        }
    }

    // Update the instance VBO with the new render data
    glBindBuffer(GL_ARRAY_BUFFER, renderVBO);
    for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
        if(this->UpdateRenderBufferRanges[y].IsEmpty()) continue;

        if(this->UpdateRenderBufferRanges[y].Start() < 0 || 
           this->UpdateRenderBufferRanges[y].End() >= CHUNK_SIZE) {
            std::cerr << "Chunk " << m_x << ", " << m_y << " has invalid render range: "
                      << this->UpdateRenderBufferRanges[y].Start() << " - "
                      << this->UpdateRenderBufferRanges[y].End() << std::endl;
            continue;
        }

        glBufferSubData(GL_ARRAY_BUFFER,
            sizeof(VoxelRenderData) * (CHUNK_SIZE * y + this->UpdateRenderBufferRanges[y].Start()), 
            sizeof(VoxelRenderData) * (this->UpdateRenderBufferRanges[y].End() - this->UpdateRenderBufferRanges[y].Start() + 1), 
            &renderData[y][this->UpdateRenderBufferRanges[y].Start()]
        );

        this->UpdateRenderBufferRanges[y].Reset();
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

void Volume::Chunk::DestroyPhysicsBody()
{
    if (b2Body_IsValid(m_physicsBody)) {
        b2DestroyBody(m_physicsBody);
        m_physicsBody = b2_nullBodyId;
    }
}

void Volume::Chunk::CreatePhysicsBody(b2WorldId worldId)
{
    if (b2Body_IsValid(m_physicsBody)) {
        std::cerr << "Physics body already exists for chunk at (" << m_x << ", " << m_y << ")." << std::endl;
        return;
    }
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = b2Vec2(m_x * CHUNK_SIZE, m_y * CHUNK_SIZE);
    m_physicsBody = b2CreateBody(worldId, &bodyDef);
}
