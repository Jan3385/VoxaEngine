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
                (chunkWorldPos.x + x),
                (chunkWorldPos.y + y)
            );
            renderData[y][x].color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // default color white
        }

        this->updatePressureBuffer = true;
        this->updateTemperatureBuffer = true;
        this->updateRenderTemperatureBuffer = true;
        this->updateVoxelBuffer = true;
        this->updateRenderBuffer = true;
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
}

/// @brief Initializes the buffers for the chunk
/// @warning Only run on the main thread. Should by called by the engine in most cases
void Volume::Chunk::InitializeBuffers()
{
    if(this->IsInitialized()){
        std::cerr << "Warning: Duplicate initialization of chunk buffers (" << m_x << ", " << m_y << ")\n";
        return;
    }

    renderVBO = Shader::GLBuffer<VoxelRenderData, GL_ARRAY_BUFFER>("Chunk Render VBO");
    VoxelRenderData tempData[CHUNK_SIZE_SQUARED];
    renderVBO.SetData(tempData, CHUNK_SIZE_SQUARED, GL_DYNAMIC_DRAW);

    renderTemperatureVBO = Shader::GLBuffer<float, GL_ARRAY_BUFFER>("Chunk Temperature VBO");
    float tempInit[CHUNK_SIZE_SQUARED] = { 0.0f };
    renderTemperatureVBO.SetData(tempInit, CHUNK_SIZE_SQUARED, GL_DYNAMIC_DRAW);

    // voxel rendering VAO setup ----
    renderVoxelVAO = Shader::GLVertexArray("Chunk Render VAO");
    renderVoxelVAO.Bind();

    renderVoxelVAO.AddAttribute<glm::vec2>(0, 2, *GameEngine::renderer->quadBuffer, GL_FALSE, 0, 0); // location 0: vec2 quad
    renderVoxelVAO.AddIntAttribute<glm::ivec2>(1, 2, renderVBO, offsetof(VoxelRenderData, position), 1); // location 1: ivec2 position
    renderVoxelVAO.AddAttribute<glm::vec4>(2, 4, renderVBO, GL_FALSE, offsetof(VoxelRenderData, color), 1); // location 2: vec4 color

    renderVoxelVAO.Unbind();
    // --------------

    // heat rendering VAO setup ----
    heatRenderingVAO = Shader::GLVertexArray("Chunk Heat VAO");
    heatRenderingVAO.Bind();

    heatRenderingVAO.AddAttribute<glm::vec2>(0, 2, *GameEngine::renderer->quadBuffer, GL_FALSE, 0, 0); // location 0: vec2 quad
    heatRenderingVAO.AddIntAttribute<glm::ivec2>(1, 2, renderVBO, offsetof(VoxelRenderData, position), 1); // location 1: ivec2 position
    heatRenderingVAO.AddAttribute<glm::vec4>(2, 4, renderVBO, GL_FALSE, offsetof(VoxelRenderData, color), 1); // location 2: vec4 color
    heatRenderingVAO.AddAttribute<float>(3, 1, renderTemperatureVBO, GL_FALSE, 0, 1); // location 3: float temperature

    heatRenderingVAO.Unbind();
    // --------------

    // Update the instance VBO with the new render data
    renderVBO.SetData(*renderData, CHUNK_SIZE_SQUARED, GL_DYNAMIC_DRAW);
}
bool Volume::Chunk::ShouldChunkDelete(AABB Camera) const
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

/*
void Volume::Chunk::UpdateComputeGPUBuffers()
{   
    // Update pressure buffer
    float pressureVBOBuffer[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];

    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        if(this->updateVoxelPressureRanges[y].IsEmpty()) continue;
        for (int x = this->updateVoxelPressureRanges[y].Start(); x <= this->updateVoxelPressureRanges[y].End(); ++x) {
            pressureVBOBuffer[y][x] = this->voxels[y][x]->amount;
        }
    }

    for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
        if(this->updateVoxelPressureRanges[y].IsEmpty()) continue;

        pressureVBO.UpdateData(
            CHUNK_SIZE * y + this->updateVoxelPressureRanges[y].Start(),
            &pressureVBOBuffer[y][this->updateVoxelPressureRanges[y].Start()],
            this->updateVoxelPressureRanges[y].End() - this->updateVoxelPressureRanges[y].Start() + 1
        );

        this->updateVoxelPressureRanges[y].Reset();
    }

    uint32_t idBuffer[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];
    float heatCapacityBuffer[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];
    float heatConductivityBuffer[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];

    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        if(this->updateVoxelIdRanges[y].IsEmpty()) continue;
        for (int x = this->updateVoxelIdRanges[y].Start(); x <= this->updateVoxelIdRanges[y].End(); ++x) {
            idBuffer[y][x] = this->voxels[y][x]->properties->id;
            heatCapacityBuffer[y][x] = this->voxels[y][x]->properties->heatCapacity;
            heatConductivityBuffer[y][x] = this->voxels[y][x]->properties->heatConductivity;
        }
    }

    for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
        if(this->updateVoxelIdRanges[y].IsEmpty()) continue;
        int rangeStart = this->updateVoxelIdRanges[y].Start();
        int rangeEnd = this->updateVoxelIdRanges[y].End();

        idVBO.UpdateData(
            CHUNK_SIZE * y + rangeStart,
            &idBuffer[y][rangeStart],
            rangeEnd - rangeStart + 1
        );
        heatCapacityVBO.UpdateData(
            CHUNK_SIZE * y + rangeStart,
            &heatCapacityBuffer[y][rangeStart],
            rangeEnd - rangeStart + 1
        );
        heatConductivityVBO.UpdateData(
            CHUNK_SIZE * y + rangeStart,
            &heatConductivityBuffer[y][rangeStart],
            rangeEnd - rangeStart + 1
        );

        this->updateVoxelIdRanges[y].Reset();
    }

    // Update the temperature buffer
    float temperatureVBOBuffer[Chunk::CHUNK_SIZE][Chunk::CHUNK_SIZE];

    for (int y = 0; y < Chunk::CHUNK_SIZE; ++y) {
        if(this->updateVoxelTemperatureRanges[y].IsEmpty()) continue;
        for (int x = this->updateVoxelTemperatureRanges[y].Start(); x <= this->updateVoxelTemperatureRanges[y].End(); ++x) {
            temperatureVBOBuffer[y][x] = this->voxels[y][x]->temperature.GetCelsius();
        }
    }

    // Update the instance VBO with the new temperature data
    for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
        if(this->updateVoxelTemperatureRanges[y].IsEmpty()) continue;
        int rangeStart = this->updateVoxelTemperatureRanges[y].Start();
        int rangeEnd = this->updateVoxelTemperatureRanges[y].End();

        temperatureVBO.UpdateData(
            CHUNK_SIZE * y + rangeStart,
            &temperatureVBOBuffer[y][rangeStart],
            rangeEnd - rangeStart + 1
        );
        renderTemperatureVBO.UpdateData(
            CHUNK_SIZE * y + rangeStart,
            &temperatureVBOBuffer[y][rangeStart],
            rangeEnd - rangeStart + 1
        );

        this->updateVoxelTemperatureRanges[y].Reset();
    }
}
*/

/// @brief Updates the compute GPU buffers for the chunk.
/// @param pressureBuffer 
/// @param temperatureBuffer 
/// @param idBuffer 
/// @param heatCapacityBuffer 
/// @param heatConductivityBuffer 
void Volume::Chunk::UpdateComputeGPUBuffers(
    Shader::GLGroupStorageBuffer<float>     &pressureBuffer, 
    Shader::GLGroupStorageBuffer<float>     &temperatureBuffer, 
    Shader::GLGroupStorageBuffer<uint32_t>  &idBuffer, 
    Shader::GLGroupStorageBuffer<float>     &heatCapacityBuffer, 
    Shader::GLGroupStorageBuffer<float>     &heatConductivityBuffer)
{
    if(this->bufferTicket == Shader::InvalidTicket){
        std::cerr << "Error: Trying to update compute GPU buffers of chunk without a valid ticket (" << m_x << ", " << m_y << ")" << std::endl;
        return;
    }

    if(updatePressureBuffer){
        float pressureBufferData[CHUNK_SIZE_SQUARED];
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                pressureBufferData[y * CHUNK_SIZE + x] = voxels[y][x]->amount;
            }
        }
        pressureBuffer.SetData(this->bufferTicket, pressureBufferData);
        updatePressureBuffer = false;
    }

    if(updateTemperatureBuffer){
        float temperatureBufferData[CHUNK_SIZE_SQUARED];
        for (int y = 0; y < CHUNK_SIZE; ++y) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                temperatureBufferData[y * CHUNK_SIZE + x] = voxels[y][x]->temperature.GetCelsius();
            }
        }
        temperatureBuffer.SetData(this->bufferTicket, temperatureBufferData);
        updateTemperatureBuffer = false;
    }

    if(updateVoxelBuffer){
        uint32_t idBufferData[CHUNK_SIZE_SQUARED];
        float heatCapacityData[CHUNK_SIZE_SQUARED];
        float heatConductivityData[CHUNK_SIZE_SQUARED];

        for(int y = 0; y < CHUNK_SIZE; ++y) {
            for(int x = 0; x < CHUNK_SIZE; ++x) {
                idBufferData[y * CHUNK_SIZE + x] = voxels[y][x]->properties->id;
                heatCapacityData[y * CHUNK_SIZE + x] = voxels[y][x]->properties->heatCapacity;
                heatConductivityData[y * CHUNK_SIZE + x] = voxels[y][x]->properties->heatConductivity;
            }
        }
        idBuffer.SetData(this->bufferTicket, idBufferData);
        heatCapacityBuffer.SetData(this->bufferTicket, heatCapacityData);
        heatConductivityBuffer.SetData(this->bufferTicket, heatConductivityData);
        updateVoxelBuffer = false;
    }
}

void Volume::Chunk::UpdateRenderGPUBuffers()
{
    if(!renderVBO.IsInitialized()) return;
    if(updateRenderBuffer){
        for(uint8_t y = 0; y < CHUNK_SIZE; ++y) {
            for(uint8_t x = 0; x < CHUNK_SIZE; ++x) {
                renderData[y][x].color = voxels[y][x]->color.getGLMVec4();
            }
        }
        renderVBO.SetData(
            &renderData[0][0],
            CHUNK_SIZE_SQUARED,
            GL_DYNAMIC_DRAW
        );
    }
}
void Volume::Chunk::SetTemperatureAt(Vec2i pos, Temperature temperature)
{
    // normalize just in case
    pos.x = pos.x % CHUNK_SIZE;
    pos.y = pos.y % CHUNK_SIZE;

    // set the temperature
    voxels[pos.y][pos.x]->temperature = temperature;

    updateTemperatureBuffer = true;
    updateRenderTemperatureBuffer = true;
}
void Volume::Chunk::SetPressureAt(Vec2i pos, float pressure)
{
    // normalize just in case
    pos.x = pos.x % CHUNK_SIZE;
    pos.y = pos.y % CHUNK_SIZE;

    // set the pressure
    voxels[pos.y][pos.x]->amount = pressure;

    updatePressureBuffer = true;
}
void Volume::Chunk::UpdatedVoxelAt(Vec2i pos)
{
    // normalize just in case
    pos.x = pos.x % CHUNK_SIZE;
    pos.y = pos.y % CHUNK_SIZE;

    // Update range
    updatePressureBuffer = true;
    updateTemperatureBuffer = true;
    updateVoxelBuffer = true;
    updateRenderBuffer = true;
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

/// @brief Resets the voxel update data for the chunk
/// @warning do not call without locking the voxel mutex
/// @note main use only for simulation thread (called automatically)
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
    this->UpdateRenderGPUBuffers();
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
