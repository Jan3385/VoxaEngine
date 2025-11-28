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
    VoxelObject(Vec2f position, std::vector<std::vector<Registry::VoxelData>> &voxelData, std::string name);
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
    virtual void UpdateCPURenderData();
    virtual unsigned int UpdateGPURenderBuffer();

    Vec2f GetPosition() const   { return position; }
    // Get rotation in radians
    float GetRotation() const   { return rotation; }

    Volume::VoxelElement* GetVoxelAt(const Vec2i& worldPos) const;
    virtual bool SetVoxelAt(const Vec2i& worldPos, Volume::VoxelElement* voxel, bool noDelete = false);

    AABB GetBoundingBox() const { return this->boundingBox;}
    void UpdateBoundingBox();

    void SetRotation(float rotation) {
        this->rotation = rotation;
        if(std::abs(this->rotation - this->bufferRotation) < ROTATION_BUFFER_THRESHOLD) return;
        this->bufferRotation = this->rotation;
        this->dirtyRotation = true;
    }

    Vec2f GetRotatedLocalPosition(const Vec2f& localPos) const;
    Vec2i GetWorldPositionFromLocalRotatedIndex(int x, int y) const;

    virtual void UpdateRotatedVoxelBuffer();
 
    Vec2i GetSize() const { return Vec2i(width, height); }

    std::string GetName() const { return name; }

    Shader::GLVertexArray renderVoxelArray;

    std::vector<std::vector<Volume::VoxelElement*>> voxels;
    // VoxelElement buffer for rotated voxels
    // Includes empty pointers for empty voxels
    std::vector<std::vector<Volume::VoxelElement*>> rotatedVoxelBuffer;
protected:
    std::string name;

    bool enabled = false;
    Vec2f position;
    int width = 0;
    int height = 0;
    AABB boundingBox;

    float ExchangeHeatBetweenVoxels(Volume::VoxelElement* v1, Volume::VoxelElement* v2);

    std::vector<Volume::VoxelRenderData> renderData;

    bool dirtyRotation = true;
    float bufferRotation = 0.0f;

    Shader::GLBuffer<Volume::VoxelRenderData, GL_ARRAY_BUFFER> renderVoxelBuffer;
private:
    static constexpr float ROTATION_BUFFER_THRESHOLD = M_PI_2 * 0.03f; // 1.5 degrees

    float rotation = 0.0f; // in radians
    unsigned int renderVoxelCount = 0;

    float maxHeatTransfer = 0.0f;
};