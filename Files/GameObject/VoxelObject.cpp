#include "VoxelObject.h"

#include "Rendering/SpriteRenderer.h"
#include "GameEngine.h"
#include "Registry/GameObjectRegistry.h"
#include "World/Voxel.h"

#include "Math/FastRotation.h"

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
                    data.id, Vec2i(x, y), 0.0f, Volume::Temperature(21.0f), true);
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
    this->enabled = false;

    std::vector<VoxelObject*>& vec = GameEngine::instance->chunkMatrix.voxelObjects;
    vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
}

void VoxelObject::Update(ChunkMatrix& chunkMatrix, float deltaTime)
{
    
}

unsigned int VoxelObject::UpdateRenderBuffer()
{
    std::vector<Volume::VoxelRenderData> renderData;

    size_t usedCount = 0;
    Vec2f offset = this->position - Vec2f((rotatedVoxelBuffer[0].size()) / 2.0f, (rotatedVoxelBuffer.size()) / 2.0f);
    offset.x = std::ceil(offset.x);
    offset.y = std::ceil(offset.y + 0.1f); // offset to avoid object being above the ground (makes the object sometimes render inside the ground)

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
                ++usedCount;
            }
        }
    }

    renderVoxelCount = static_cast<unsigned int>(usedCount);

    glBindBuffer(GL_ARRAY_BUFFER, renderVoxelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Volume::VoxelRenderData) * renderVoxelCount, renderData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return renderVoxelCount;
}

Volume::VoxelElement *VoxelObject::GetVoxelAt(const Vec2i &worldPos) const
{
    Vec2i localPos = worldPos - Vec2i(this->position) + Vec2i(this->width / 2, this->height / 2);

    if (localPos.x < 0 || localPos.x >= static_cast<int>(this->rotatedVoxelBuffer[0].size()) || 
        localPos.y < 0 || localPos.y >= static_cast<int>(this->rotatedVoxelBuffer.size())) {
        return nullptr;
    }
    
    return this->rotatedVoxelBuffer[localPos.y][localPos.x];
}

void VoxelObject::SetVoxelAt(const Vec2i &worldPos, Volume::VoxelElement *voxel)
{
    Vec2i localPos = worldPos - Vec2i(this->position) + Vec2i(this->width / 2, this->height / 2);

    // get the correct position from the rotated buffer
    if(this->rotatedVoxelBuffer[localPos.y][localPos.x] == nullptr) {
        return; // No voxel at this position TODO: somehow recalculate the position instead of returning
    }
    localPos = this->rotatedVoxelBuffer[localPos.y][localPos.x]->position;

    if (localPos.x < 0 || localPos.x >= this->width || localPos.y < 0 || localPos.y >= this->height) {
        return; // Out of bounds
    }

    if (voxel) {
        voxel->position = localPos;
    }

    // Delete the old voxel if it exists
    if (this->voxels[localPos.y][localPos.x]) {
        delete this->voxels[localPos.y][localPos.x];
    }

    this->voxels[localPos.y][localPos.x] = voxel;

    // Update bounding box
    this->UpdateBoundingBox(); //TODO: mark as dirty instead
    this->dirtyRotation = true;
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
        FastRotate2DVector(this->voxels, this->rotatedVoxelBuffer, -this->rotation);
        this->dirtyRotation = false;
    }
}
