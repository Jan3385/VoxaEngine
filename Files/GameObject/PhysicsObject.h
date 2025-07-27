#pragma once

#include <GameObject/VoxelObject.h>

#include <box2d/box2d.h>

struct Triangle;

class PhysicsObject: public VoxelObject
{
public:
    PhysicsObject() = default;
    PhysicsObject(Vec2f position, std::vector<std::vector<Registry::VoxelData>> &voxelData)
        : VoxelObject(position, voxelData) { }

    ~PhysicsObject();

    // Disable copy and move semantics
    PhysicsObject(const PhysicsObject&) = delete;
    PhysicsObject(PhysicsObject&&) = delete;
    PhysicsObject& operator=(const PhysicsObject&) = delete;
    PhysicsObject& operator=(PhysicsObject&&) = delete;

    void SetVoxelAt(const Vec2i& worldPos, Volume::VoxelElement* voxel) override;

    bool dirtyColliders = true;
	void UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId);

    void UpdatePhysicPosition(b2WorldId worldId);

    // physics mesh body
    std::vector<Triangle> triangleColliders;
	std::vector<b2Vec2> edges;
private:
    b2BodyId m_physicsBody = b2_nullBodyId;

    b2Vec2 velocity = b2Vec2(0.0f, 0.0f);
    float angularVelocity = 0.0f;

    void CreatePhysicsBody(b2WorldId worldId);
    void DestroyPhysicsBody();
};
