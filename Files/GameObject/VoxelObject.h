#pragma once

#include <SDL.h>
#include <GL/glew.h>

#include "Math/Vector.h"
#include "Math/AABB.h"
#include "World/Chunk.h"

#include <iostream>

namespace Registry {
    struct VoxelData;
}

class VoxelObject{
public:
    VoxelObject() = default;
    VoxelObject(Vec2f position, std::vector<std::vector<Registry::VoxelData>> &voxelData);
    virtual ~VoxelObject();

    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return this->enabled; }

    // Disable copy and move semantics
    VoxelObject(const VoxelObject&) = delete;
    VoxelObject(VoxelObject&&) = delete;
    VoxelObject& operator=(const VoxelObject&) = delete;
    VoxelObject& operator=(VoxelObject&&) = delete;

    virtual bool Update(ChunkMatrix& chunkMatrix);

    virtual bool ShouldRender() const { return true; };
    virtual unsigned int UpdateRenderBuffer();

    Vec2f GetPosition() const   { return position; }
    // Get rotation in radians
    float GetRotation() const   { return rotation; }

    Volume::VoxelElement* GetVoxelAt(const Vec2i& worldPos) const;
    virtual bool SetVoxelAt(const Vec2i& worldPos, Volume::VoxelElement* voxel, bool noDelete = false);

    AABB GetBoundingBox() const { return this->boundingBox;}
    void UpdateBoundingBox();

    void SetRotation(float rotation) {
        if(this->rotation == rotation) return;
        this->rotation = rotation;
        this->dirtyRotation = true;
    }

    Vec2f GetRotatedLocalPosition(const Vec2f& localPos) const;

    virtual void UpdateRotatedVoxelBuffer();
 
    Vec2i GetSize() const { return Vec2i(width, height); }

	GLuint renderVoxelVAO = 0;

    std::vector<std::vector<Volume::VoxelElement*>> voxels;
protected:
    bool enabled = false;
    Vec2f position;
    int width = 0;
    int height = 0;
    AABB boundingBox;

    float ExchangeHeatBetweenVoxels(Volume::VoxelElement* v1, Volume::VoxelElement* v2);

    // VoxelElement buffer for rotated voxels
    // Includes empty pointers for empty voxels
    std::vector<std::vector<Volume::VoxelElement*>> rotatedVoxelBuffer;

    bool dirtyRotation = true;

    GLuint renderVoxelVBO = 0;
private:
    float rotation = 0.0f; // in radians
    unsigned int renderVoxelCount = 0;

    float maxHeatTransfer = 0.0f;
};