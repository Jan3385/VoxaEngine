#include "VoxelObject.h"

#include "Rendering/SpriteRenderer.h"
#include "GameEngine.h"
#include "Registry/GameObjectRegistry.h"
#include "World/Voxel.h"

#include <iostream>
#include <algorithm>

VoxelObject::VoxelObject(Vec2f position, std::vector<std::vector<Registry::VoxelData>> &voxelData)
{
    this->position = position;

    this->voxels.resize(voxelData.size());
    for (size_t y = 0; y < voxelData.size(); ++y)
    {
        this->voxels[y].resize(voxelData[y].size());
        for (size_t x = 0; x < voxelData[y].size(); ++x) {
            const auto &data = voxelData[y][x];
            if (data.id.empty()) {
                this->voxels[y][x] = nullptr; // Empty voxel
            } else {
                this->voxels[y][x] = CreateVoxelElement(
                    data.id, 
                    Vec2i(x, y), 
                    20.0f, 
                    Volume::Temperature(21.0f), 
                    true
                );

                this->voxels[y][x]->color = data.color;
            }
        }
    }

    Vec2i size = Vec2i(static_cast<int>(this->voxels[0].size()), static_cast<int>(this->voxels.size()));
    this->width = size.x;
    this->height = size.y;

    this->enabled = true;

    // Create VAO and VBO for rendering
    glGenVertexArrays(1, &renderVoxelVAO);
    glBindVertexArray(renderVoxelVAO);

    glBindBuffer(GL_ARRAY_BUFFER, GameEngine::instance->renderer->quadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &renderVoxelVBO);
    glBindBuffer(GL_ARRAY_BUFFER, renderVoxelVBO);
    glVertexAttribIPointer(1, 2, GL_INT, sizeof(Volume::VoxelRenderData), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // instance attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Volume::VoxelRenderData), (void*)(sizeof(glm::ivec2)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    
    glBindVertexArray(0);

    this->UpdateRotatedVoxelBuffer();
    this->UpdateRenderBuffer();
}

VoxelObject::~VoxelObject()
{
    if (renderVoxelVAO != 0) {
        glDeleteVertexArrays(1, &renderVoxelVAO);
        renderVoxelVAO = 0;
    }

    if (renderVoxelVBO != 0) {
        glDeleteBuffers(1, &renderVoxelVBO);
        renderVoxelVBO = 0;
    }
    
    for(int y = this->voxels.size() - 1; y >= 0; --y) {
        for(int x = this->voxels[y].size() - 1; x >= 0; --x) {
            Volume::VoxelElement* voxel = this->voxels[y][x];
            if(voxel) {
                delete voxel;
                this->voxels[y][x] = nullptr;
            }
        }
    }

    this->enabled = false;
}

/// @brief Updates the voxelobject and the voxels inside it. Should be in simulation thread
/// @param chunkMatrix 
/// @return returns true if the object should presist, false if it should be removed
bool VoxelObject::Update(ChunkMatrix& chunkMatrix)
{
    bool calculateHeat = GameEngine::instance->runHeatSimulation && maxHeatTransfer > 0.1f;
    if(calculateHeat) maxHeatTransfer = 0.0f;

    PhysicsObject* thisPhys = dynamic_cast<PhysicsObject*>(this);

    bool foundVoxel = false;

    // interaction between object and its own voxels
    for(int y = 0; y < static_cast<int>(this->voxels.size()); ++y) {
        for(int x = 0; x < static_cast<int>(this->voxels[y].size()); ++x) {
            Volume::VoxelElement* voxel = this->voxels[y][x];
            if(voxel) {
                foundVoxel = true;

                voxel->Step(&chunkMatrix);
                
                // heat transfer between voxels
                if(calculateHeat){
                    std::string transitionId = voxel->ShouldTransitionToID();
                    if(!transitionId.empty()){
                        float amount = voxel->amount;
                        Volume::Temperature temp = voxel->temperature;
                        delete this->voxels[y][x];

                        this->voxels[y][x] = CreateVoxelElement(
                            transitionId, 
                            Vec2i(x, y), 
                            amount, 
                            temp, 
                            true
                        );

                        voxel = this->voxels[y][x];
                        this->dirtyRotation = true;

                        // remove any non-solid voxels and let them enter the simulation
                        if(voxel->GetState() != Volume::State::Solid) {
                            Vec2i worldPos = Vec2i(
                                static_cast<int>(this->position.x + x - (this->rotatedVoxelBuffer[0].size() / 2.0f)),
                                static_cast<int>(this->position.y + y - (this->rotatedVoxelBuffer.size() / 2.0f))
                            );
                            voxel->position = worldPos;

                            chunkMatrix.PlaceVoxelAt(
                                voxel,
                                false,
                                false
                            );

                            this->voxels[y][x] = nullptr;
                            this->dirtyRotation = true;
                            if(thisPhys)
                                thisPhys->dirtyColliders = true;

                            continue;
                        }
                    }
                    for(Vec2i dir : vector::AROUND4){
                        if(x + dir.x < 0 || x + dir.x >= this->width ||
                           y + dir.y < 0 || y + dir.y >= this->height) {
                            continue; // Out of bounds
                        }

                        Volume::VoxelElement* neighbor = this->voxels[y + dir.y][x + dir.x];

                        if(!neighbor) continue;
                            
                        maxHeatTransfer = std::max(maxHeatTransfer, ExchangeHeatBetweenVoxels(voxel, neighbor));
                    }
                }
            }
        }
    }

    // interaction between object and chunk matrix
    for(int y = 0; y < static_cast<int>(this->rotatedVoxelBuffer.size()); ++y) {
        for(int x = 0; x < static_cast<int>(this->rotatedVoxelBuffer[y].size()); ++x) {
            Volume::VoxelElement* objectVoxel = this->rotatedVoxelBuffer[y][x];
            
            Vec2i worldPos = Vec2i(
                static_cast<int>(this->position.x + x - (this->rotatedVoxelBuffer[0].size() / 2.0f)),
                static_cast<int>(this->position.y + y - (this->rotatedVoxelBuffer.size() / 2.0f))
            );

            Volume::VoxelElement* worldVoxel = chunkMatrix.VirtualGetAt(worldPos, false);

            maxHeatTransfer = std::max(maxHeatTransfer, ExchangeHeatBetweenVoxels(objectVoxel, worldVoxel));

        }
    }

    if(!foundVoxel) {
        return false;
    }
    return true;
}

unsigned int VoxelObject::UpdateRenderBuffer()
{
    std::vector<Volume::VoxelRenderData> renderData;

    Vec2f offset = this->position - Vec2f((rotatedVoxelBuffer[0].size()) / 2.0f, (rotatedVoxelBuffer.size()) / 2.0f);
    offset.x = std::ceil(offset.x);
    offset.y = std::ceil(offset.y);

    for (size_t y = 0; y < this->rotatedVoxelBuffer.size(); ++y) {
        for (size_t x = 0; x < this->rotatedVoxelBuffer[0].size(); ++x) {
            Volume::VoxelElement* voxel = this->rotatedVoxelBuffer[y][x];
            if (voxel) {
                renderData.push_back({
                    .position = glm::ivec2(
                        x + offset.x, 
                        y + offset.y),
                    .color = voxel->color.getGLMVec4(),
                });
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, renderVoxelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Volume::VoxelRenderData) * renderData.size(), renderData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return renderData.size();
}

Volume::VoxelElement *VoxelObject::GetVoxelAt(const Vec2i &worldPos) const
{
    // True center of the buffer
    float centerX = (static_cast<float>(this->rotatedVoxelBuffer[0].size()) - 1) / 2.0f;
    float centerY = (static_cast<float>(this->rotatedVoxelBuffer.size()) - 1) / 2.0f;

    float localX = static_cast<float>(worldPos.x) - this->position.x + centerX;
    float localY = static_cast<float>(worldPos.y) - this->position.y + centerY;

    int ix = static_cast<int>(std::round(localX));
    int iy = static_cast<int>(std::round(localY));

    if (ix < 0 || ix >= static_cast<int>(this->rotatedVoxelBuffer[0].size()) || 
        iy < 0 || iy >= static_cast<int>(this->rotatedVoxelBuffer.size())) {
        return nullptr;
    }
    return this->rotatedVoxelBuffer[iy][ix];
}

/// @brief Sets a voxel at the specified world position.
/// @param worldPos The world position to set the voxel at.
/// @param voxel The voxel to set.
/// @param noDelete If true, the existing voxel at the position will not be deleted. WARNING: This may cause memory leaks.
/// @return True if the voxel was set successfully, false otherwise.
bool VoxelObject::SetVoxelAt(const Vec2i &worldPos, Volume::VoxelElement *voxel, bool noDelete)
{
    // True center of the buffer
    float centerX = (static_cast<float>(this->rotatedVoxelBuffer[0].size()) - 1) / 2.0f;
    float centerY = (static_cast<float>(this->rotatedVoxelBuffer.size()) - 1) / 2.0f;

    float localX = static_cast<float>(worldPos.x) - this->position.x + centerX;
    float localY = static_cast<float>(worldPos.y) - this->position.y + centerY;

    int ix = static_cast<int>(std::round(localX));
    int iy = static_cast<int>(std::round(localY));

    if (ix < 0 || ix >= static_cast<int>(this->rotatedVoxelBuffer[0].size()) || 
        iy < 0 || iy >= static_cast<int>(this->rotatedVoxelBuffer.size())) {
        return false; // Out of bounds
    }

    // get the correct position from the rotated buffer
    if(this->rotatedVoxelBuffer[iy][ix] == nullptr) {
        return false; // No voxel at this position TODO: somehow recalculate the position instead of returning
    }
    Vec2i localPos = this->rotatedVoxelBuffer[iy][ix]->position;

    if (localPos.x < 0 || localPos.x >= this->width || localPos.y < 0 || localPos.y >= this->height) {
        return false; // Out of bounds
    }

    voxel->position = localPos;

    // Delete the old voxel if it exists
    if (this->voxels[localPos.y][localPos.x] && !noDelete) {
        delete this->voxels[localPos.y][localPos.x];
    }

    this->voxels[localPos.y][localPos.x] = voxel;
    this->rotatedVoxelBuffer[iy][ix] = voxel;

    this->dirtyRotation = true;

    //force heat updates
    this->maxHeatTransfer = 1.0f;

    return true;
}

void VoxelObject::UpdateBoundingBox()
{
    Vec2f size = Vec2f(rotatedVoxelBuffer[0].size(), rotatedVoxelBuffer.size());
    Vec2f pos = this->position - size / 2.0f;

    this->boundingBox = AABB(pos, size);
}

void VoxelObject::UpdateRotatedVoxelBuffer()
{
    if (this->dirtyRotation) {
        this->rotatedVoxelBuffer = this->voxels;
        this->dirtyRotation = false;
    }
}

float VoxelObject::ExchangeHeatBetweenVoxels(Volume::VoxelElement *v1, Volume::VoxelElement *v2)
{
    if(!v1 || !v2) return 0.0f;

    float heatCapacity = v1->properties->HeatCapacity / 40;
    float heatDiff = v2->temperature.GetCelsius() - v1->temperature.GetCelsius();
    float heatTransfer = heatDiff * v2->properties->HeatConductivity / heatCapacity;
    
    if (heatTransfer != 0.0f) {
        v1->temperature.SetCelsius(v1->temperature.GetCelsius() + heatTransfer);
        v2->temperature.SetCelsius(v2->temperature.GetCelsius() - heatTransfer);
    }

    return heatTransfer;
}

Vec2f VoxelObject::GetRotatedLocalPosition(const Vec2f &localPos) const
{
    // Calculate the center of the voxel object
    float centerX = (static_cast<float>(this->width) - 1) / 2.0f;
    float centerY = (static_cast<float>(this->height) - 1) / 2.0f;

    // Translate localPos to be relative to the center
    float relX = localPos.x - centerX;
    float relY = localPos.y - centerY;

    float cos = std::cos(this->rotation);
    float sin = std::sin(this->rotation);

    // Rotate around the center
    float rotatedX = cos * relX - sin * relY;
    float rotatedY = sin * relX + cos * relY;

    // Translate back
    return Vec2f(rotatedX + centerX, rotatedY + centerY);
}
