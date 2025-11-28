#include "VoxelObject.h"

#include "Rendering/SpriteRenderer.h"
#include "GameEngine.h"
#include "Registry/VoxelObjectRegistry.h"
#include "World/Voxel.h"
#include "Shader/GLVertexArray.h"

#include <iostream>
#include <algorithm>

VoxelObject::VoxelObject(Vec2f position, std::vector<std::vector<Registry::VoxelData>> &voxelData, std::string name)
{
    this->position = position;
    this->name = name;

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
                this->voxels[y][x]->partOfObject = true;
                this->voxels[y][x]->color = data.color;
            }
        }
    }

    Vec2i size = Vec2i(static_cast<int>(this->voxels[0].size()), static_cast<int>(this->voxels.size()));
    this->width = size.x;
    this->height = size.y;

    this->enabled = true;

    // Create VAO and VBO for rendering
    renderVoxelArray = Shader::GLVertexArray("VoxelObject VAO");
    renderVoxelArray.Bind();

    renderVoxelArray.AddAttribute<glm::vec2>(0, 2, *GameEngine::renderer->quadBuffer, GL_FALSE, 0, 0);                           // location 0: vec2 texCoord

    renderVoxelBuffer = Shader::GLBuffer<Volume::VoxelRenderData, GL_ARRAY_BUFFER>("VoxelObject VBO");
    renderVoxelArray.AddIntAttribute<glm::ivec2>(1, 2, renderVoxelBuffer, offsetof(Volume::VoxelRenderData, position), 1);      // location 1: vec2 worldPos
    renderVoxelArray.AddAttribute<glm::vec4>(2, 4, renderVoxelBuffer, GL_FALSE, offsetof(Volume::VoxelRenderData, color), 1);   // location 2: vec4 color

    renderVoxelArray.Unbind();

    this->UpdateRotatedVoxelBuffer();
    this->UpdateCPURenderData();
}

VoxelObject::~VoxelObject()
{
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
    bool runHeatSim = GameEngine::instance->runHeatSimulation;
    bool runChemSim = GameEngine::instance->runChemicalReactions;
    bool calculateHeat = runHeatSim && maxHeatTransfer > 0.1f;

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
            if(!objectVoxel) continue;

            Vec2i worldPos = Vec2i(
                static_cast<int>(this->position.x + x - (this->rotatedVoxelBuffer[0].size() - 1) / 2),
                static_cast<int>(this->position.y + y - (this->rotatedVoxelBuffer.size() - 1) / 2)
            );

            Volume::VoxelElement* worldVoxel = chunkMatrix.VirtualGetAt(worldPos, false);
            if(!worldVoxel) continue;

            if(runHeatSim) maxHeatTransfer = std::max(maxHeatTransfer, ExchangeHeatBetweenVoxels(objectVoxel, worldVoxel));

            if(runChemSim) {
                for (Registry::ChemicalReactionProperty reaction : objectVoxel->properties->Reactions)
                {
                    if(reaction.minTemperature.GetCelsius() > worldVoxel->temperature.GetCelsius()) continue;
                    if(reaction.catalyst != worldVoxel->properties->id) continue;
                    
                    float random = Volume::voxelRandomGenerator.GetFloat(0.0f, 1.0f);
                    if(reaction.reactionSpeed < random) break;

                    Volume::VoxelElement* newVoxel = CreateVoxelElement(
                        reaction.to,
                        worldPos,
                        objectVoxel->amount,
                        objectVoxel->temperature,
                        true
                    );
                    this->SetVoxelAt(
                        worldPos,
                        newVoxel
                    );

                    if(reaction.preserveCatalyst) break;
                    chunkMatrix.PlaceVoxelAt(
                        worldPos,
                        reaction.to,
                        worldVoxel->temperature,
                        worldVoxel->IsUnmoveableSolid(),
                        worldVoxel->amount,
                        true,
                        false
                    );
                }
                
            }
        }
    }

    this->UpdateCPURenderData();

    if(!foundVoxel) {
        return false;
    }
    return true;
}

void VoxelObject::UpdateCPURenderData()
{
    std::lock_guard<std::mutex> lock(GameEngine::instance->openGLMutex);
    
    this->renderData.clear();

    Vec2f offset = this->position - Vec2f((rotatedVoxelBuffer[0].size()) / 2.0f, (rotatedVoxelBuffer.size()) / 2.0f);
    offset.x = std::ceil(offset.x);
    offset.y = std::ceil(offset.y);

    for (size_t y = 0; y < this->rotatedVoxelBuffer.size(); ++y) {
        for (size_t x = 0; x < this->rotatedVoxelBuffer[0].size(); ++x) {
            Volume::VoxelElement* voxel = this->rotatedVoxelBuffer[y][x];
            if (voxel) {
                this->renderData.push_back({
                    .position = glm::ivec2(
                        x + offset.x, 
                        y + offset.y),
                    .color = voxel->color.getGLMVec4(),
                });
            }
        }
    }
}

unsigned int VoxelObject::UpdateGPURenderBuffer()
{
    renderVoxelBuffer.SetData(renderData, GL_DYNAMIC_DRAW);
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
        return false; // No voxel at this position
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

    voxel->partOfObject = true;
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

    float heatCapacity = v1->properties->heatCapacity / 40;
    float heatDiff = v2->temperature.GetCelsius() - v1->temperature.GetCelsius();
    float heatTransfer = heatDiff * v2->properties->heatConductivity / heatCapacity;
    
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

Vec2i VoxelObject::GetWorldPositionFromLocalRotatedIndex(int x, int y) const
{
    // Calculate the center of the voxel object
    float centerX = (static_cast<float>(this->rotatedVoxelBuffer[0].size()) - 1) / 2.0f;
    float centerY = (static_cast<float>(this->rotatedVoxelBuffer.size()) - 1) / 2.0f;

    // Translate local index to be relative to the center
    float relX = x - centerX;
    float relY = y - centerY;

    Vec2i worldPos = Vec2i(static_cast<int>(std::round(relX + this->position.x)),
                           static_cast<int>(std::round(relY + this->position.y)));

    return worldPos;
}
