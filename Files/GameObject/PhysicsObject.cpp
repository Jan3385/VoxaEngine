#include "PhysicsObject.h"

#include <iostream>
#include <algorithm>

#include "Physics/Physics.h"
#include "GameEngine.h"

PhysicsObject::~PhysicsObject()
{
    if (b2Body_IsValid(m_physicsBody)) {
        b2DestroyBody(m_physicsBody);
        m_physicsBody = b2_nullBodyId;
    }

    std::vector<PhysicsObject*>& vec = GameEngine::instance->physics->physicsObjects;
    vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());

    VoxelObject::~VoxelObject();
}

void PhysicsObject::UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId)
{
    this->dirtyColliders = false;

    this->DestroyPhysicsBody();
    this->CreatePhysicsBody(worldId);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material = b2DefaultSurfaceMaterial();

    b2Vec2 centerOffset = b2Vec2(
        (this->width - 1) / 2.0f, 
        (this->height) / 2.0f);

    for(Triangle t : triangles){
        b2Hull hull;
        hull.points[0] = t.a - centerOffset;
        hull.points[1] = t.b - centerOffset;
        hull.points[2] = t.c - centerOffset;
        hull.count = 3;

        b2Polygon polygon = b2MakePolygon(
            &hull, 0.01f
        );

        b2CreatePolygonShape(
            m_physicsBody, &shapeDef, &polygon
        );
    }

    this->triangleColliders.assign(triangles.begin(), triangles.end());
    this->edges.assign(edges.begin(), edges.end());
}

void PhysicsObject::Update(ChunkMatrix& chunkMatrix, float deltaTime)
{
    VoxelObject::Update(chunkMatrix, deltaTime);
}

void PhysicsObject::UpdatePhysicPosition(b2WorldId worldId)
{
    if (!b2Body_IsValid(m_physicsBody)) {
        return;
    }

    b2Vec2 newPos = b2Body_GetPosition(m_physicsBody);
    this->position = Vec2f(newPos.x, newPos.y);
    b2Rot rot = b2Body_GetRotation(m_physicsBody);
    this->SetRotation(std::atan2(rot.s, rot.c)); // Convert b2Rot to radians

    this->UpdateBoundingBox();
}

void PhysicsObject::DestroyPhysicsBody()
{
    if (b2Body_IsValid(m_physicsBody)) {
        b2DestroyBody(m_physicsBody);
        m_physicsBody = b2_nullBodyId;
    }
}
void PhysicsObject::CreatePhysicsBody(b2WorldId worldId)
{
    if (b2Body_IsValid(m_physicsBody)) {
        std::cerr << "Physics body already exists for Physics Body at (" << this->position.x << ", " << this->position.y << ")." << std::endl;
        return;
    }
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;

    std::string name = "PhysOBJ:X" + std::to_string(this->position.x) + "_Y" + std::to_string(this->position.y);
    // Circumsize the name to fit within 31 characters
    if (name.length() > 31) {
        name = name.substr(0, 31);
    }
    bodyDef.name = name.c_str();

    bodyDef.isAwake = true;
    bodyDef.isEnabled = true;
    bodyDef.rotation = b2MakeRot(0.0f);
    bodyDef.enableSleep = true;
    // Set origin to center of object
    bodyDef.position = b2Vec2(position.x, position.y);
    m_physicsBody = b2CreateBody(worldId, &bodyDef);
}