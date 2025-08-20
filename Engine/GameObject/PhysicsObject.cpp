#include "PhysicsObject.h"

#include "Math/FastRotation.h"

#include <iostream>
#include <algorithm>

#include "Physics/Physics.h"
#include "GameEngine.h"

PhysicsObject::~PhysicsObject()
{
    this->DestroyPhysicsBody();
}

void PhysicsObject::UpdatePhysicsEffects(ChunkMatrix &chunkMatrix, float deltaTime)
{
    // RIP, bouncy barrels will never be forgotten 🪦🪽✝️

    int ySize = static_cast<int>(this->rotatedVoxelBuffer.size());

    Vec2i bottomOfObject = Vec2i(
        static_cast<int>(this->position.x), 
        static_cast<int>(this->position.y + ySize / 2.0f - 1)
    );

    float percentageOfObjectInLiquid = 0.0f;
    for(int y = 0; y < ySize; ++y) {
        Volume::VoxelElement* voxel = chunkMatrix.VirtualGetAt( Vec2i(bottomOfObject.x, bottomOfObject.y - y), false);

        if(voxel && voxel->GetState() == Volume::State::Liquid)
            percentageOfObjectInLiquid += 1.0f / ySize;
    }

    if(percentageOfObjectInLiquid > 0.0f){
        // inflate the percentage a little bit to make objects more above water when calm
        percentageOfObjectInLiquid = std::clamp(percentageOfObjectInLiquid + 0.2f, 0.0f, 1.0f);

        float mass = b2Body_GetMass(m_physicsBody);

        // apply buoyancy force
        constexpr float buoyancyFactor = 1.1f;
        b2Vec2 buoyancyForce = b2Vec2(0, -9.81f * mass * buoyancyFactor * percentageOfObjectInLiquid);
        b2Body_ApplyForceToCenter(m_physicsBody, buoyancyForce, true);

        // apply drag force
        b2Vec2 dragForce = -b2Body_GetLinearVelocity(m_physicsBody) * 0.1f * mass;
        b2Body_ApplyForceToCenter(m_physicsBody, dragForce, false);

        // apply angular drag
        float angularVelocity = b2Body_GetAngularVelocity(m_physicsBody);
        float angularDragCoefficient = 5.0f * percentageOfObjectInLiquid;
        float angularDragTorque = -angularVelocity * angularDragCoefficient * mass;
        b2Body_ApplyTorque(m_physicsBody, angularDragTorque, false);
    }
}

bool PhysicsObject::SetVoxelAt(const Vec2i &worldPos, Volume::VoxelElement *voxel, bool noDelete)
{
    this->dirtyColliders = true;
    return VoxelObject::SetVoxelAt(worldPos, voxel, noDelete);
}

void PhysicsObject::UpdateColliders(std::vector<Triangle> &triangles, std::vector<b2Vec2> &edges, b2WorldId worldId)
{   
    this->dirtyColliders = false;

    this->DestroyPhysicsBody();
    this->CreatePhysicsBody(worldId);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material = b2DefaultSurfaceMaterial();

    float avarageDensity = densityOverride;
    if(densityOverride == 0.0f){
        for (const auto& row : this->voxels) {
            for (const auto& voxel : row) {
                if (voxel) {
                    avarageDensity += voxel->properties->Density * voxel->amount;
                }
            }
        }
        avarageDensity /= this->GetSize().x * this->GetSize().y;
        avarageDensity *= Volume::VOXEL_SIZE_METERS;
    }

    shapeDef.density = avarageDensity;

    b2Vec2 centerOffset = b2Vec2(
        (this->width) / 2.0f, 
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

void PhysicsObject::UpdateRotatedVoxelBuffer()
{
    if (this->dirtyRotation) {
        FastRotate2DVector(this->voxels, this->rotatedVoxelBuffer, -this->GetRotation());
        this->dirtyRotation = false;
    }
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
        // Store the velocity and angular velocity before destroying the body
        this->velocity = b2Body_GetLinearVelocity(m_physicsBody);
        this->angularVelocity = b2Body_GetAngularVelocity(m_physicsBody);

        b2DestroyBody(m_physicsBody);
        m_physicsBody = b2_nullBodyId;
    }
}
void PhysicsObject::CreatePhysicsBody(b2WorldId worldId)
{
    //FIXME: linux cant create the physics body for some reason
    if (b2Body_IsValid(m_physicsBody)) {
        std::cerr << "Physics body already exists for Physics Body at (" << this->position.x << ", " << this->position.y << ")." << std::endl;
        return;
    }
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;

    //std::string name = "PhysOBJ:X" + std::to_string(this->position.x) + "_Y" + std::to_string(this->position.y);
    //// Circumsize the name to fit within 31 characters
    //if (name.length() > 31) {
    //    name = name.substr(0, 31);
    //}
    //bodyDef.name = name.c_str();

    bodyDef.isAwake = true;
    bodyDef.isEnabled = true;
    bodyDef.rotation = b2MakeRot(this->GetRotation());
    bodyDef.enableSleep = true;

    bodyDef.fixedRotation = this->IsAbleToRotate() ? false : true;
    
    bodyDef.angularVelocity = this->angularVelocity;
    bodyDef.linearVelocity = this->velocity;

    // Set origin to center of object
    bodyDef.position = b2Vec2(position.x, position.y);
    this->m_physicsBody = b2CreateBody(worldId, &bodyDef);

    assert(b2Body_IsValid(this->m_physicsBody) && "Failed to create physics body for object");
}