#pragma once

#include "GameObject/VoxelObject.h"
#include "Physics/Triangle.h"

#include <box2d/box2d.h>

class PhysicsObject: public VoxelObject
{
public:
    PhysicsObject() = default;
    PhysicsObject(Vec2f position, std::vector<std::vector<Registry::VoxelData>> &voxelData, 
        float densityOverride, std::string name)
        : VoxelObject(position, voxelData, name), densityOverride(densityOverride) { }

    ~PhysicsObject();

    // Disable copy and move semantics
    PhysicsObject(const PhysicsObject&) = delete;
    PhysicsObject(PhysicsObject&&) = delete;
    PhysicsObject& operator=(const PhysicsObject&) = delete;
    PhysicsObject& operator=(PhysicsObject&&) = delete;

    void UpdatePhysicsEffects(ChunkMatrix& chunkMatrix, float deltaTime);

    bool SetVoxelAt(const Vec2i& worldPos, Volume::VoxelElement* voxel, bool noDelete = false) override;

    virtual bool CanBreakIntoParts() const { return true; }

    b2BodyId GetPhysicsBodyId() const { return m_physicsBody; }

    bool dirtyColliders = true;
	virtual void UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId);

    void UpdateRotatedVoxelBuffer() override;

    void UpdatePhysicPosition(b2WorldId worldId);

    // physics mesh body
    std::vector<Triangle> triangleColliders;
	std::vector<b2Vec2> edges;
protected:
    virtual bool IsAbleToRotate() const { return true; }

    b2BodyId m_physicsBody = b2_nullBodyId;

    void CreatePhysicsBody(b2WorldId worldId);
    void DestroyPhysicsBody();

    float densityOverride = 0.0f;
private:
    b2Vec2 velocity = b2Vec2(0.0f, 0.0f);
    float angularVelocity = 0.0f;
};
