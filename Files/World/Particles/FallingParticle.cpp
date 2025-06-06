#include "FallingParticle.h"
#include "World/ChunkMatrix.h"

#include <cmath>

Particle::FallingParticle::FallingParticle() :
    VoxelParticle(),
    m_dPosition(0, 0)
{
    this->fPosition = Vec2f(0, 0);
    this->color = RGBA(255, 255, 255, 255);
}

Particle::FallingParticle::FallingParticle(Vec2f position, RGBA color, float angle, float speed, float gravityMultiplier) :
    VoxelParticle(),
    m_dPosition(speed * cos(angle), speed * sin(angle)),
    gravityMultiplier(gravityMultiplier)
{
    this->fPosition = position;
    this->color = color;
}

Particle::FallingParticle::~FallingParticle()
{
}

bool Particle::FallingParticle::Step(ChunkMatrix *matrix)
{ 
    //new position variables
    this->fPosition += m_dPosition;

    //Adjust position according to gravity
    m_dPosition = m_dPosition + Vec2f(0, Particle::GRAVITY * this->gravityMultiplier); // Apply gravity

    Vec2f futurePos = fPosition + m_dPosition;

    Volume::VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos);
    if (!futureVoxel || futureVoxel->GetState() == Volume::State::Solid || this->ShouldDie())
    {
    	return true;
    }

    if(!isTimeImmortal)
        this->particleLifeTime--;

    return false;
}

Vec2f Particle::FallingParticle::GetPosition() const
{
    return Vec2f((Vec2i)this->fPosition);
}

Particle::FallingParticle* Particle::AddFallingParticle(ChunkMatrix *matrix, 
    RGBA color, float angle, 
    float speed, float gravityMultiplier,
    Vec2f position, float particleLifetime)
{
    FallingParticle *particle = new FallingParticle(position, color, angle, speed, gravityMultiplier);
    particle->particleLifeTime = particleLifetime;

    matrix->newParticles.push_back(particle);

    return particle;
}
