#pragma once

#include <GameObject/GameObject.h>

#include <box2d/box2d.h>

struct Triangle;

class PhysicsObject: public GameObject
{
public:
    PhysicsObject() = default;
    PhysicsObject(Vec2f position, std::string texturePath)
        : GameObject(position, texturePath) { }

    ~PhysicsObject();

    // Disable copy and move semantics
    PhysicsObject(const PhysicsObject&) = delete;
    PhysicsObject(PhysicsObject&&) = delete;
    PhysicsObject& operator=(const PhysicsObject&) = delete;
    PhysicsObject& operator=(PhysicsObject&&) = delete;

    bool dirtyColliders = true;
	void UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId);

    void Update(ChunkMatrix& chunkMatrix, float deltaTime) override;
    void UpdatePhysicPosition(b2WorldId worldId);
    void CreatePhysicsBody(b2WorldId worldId);
private:
    b2BodyId m_physicsBody = b2_nullBodyId;
    std::vector<Triangle> m_triangleColliders;
	std::vector<b2Vec2> m_edges;

    void DestroyPhysicsBody();
};
