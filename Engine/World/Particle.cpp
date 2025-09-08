#include "World/Particle.h"
#include "Particle.h"

#include "World/ChunkMatrix.h"

using namespace Particle;

VoxelParticle::VoxelParticle()
{
    this->position = Vec2f(0, 0);
    this->color = RGBA(255, 0, 255, 255);
}

VoxelParticle::VoxelParticle(const Vec2f &position, RGBA color)
{
    this->position = position;
    this->color = color;
}

void Particle::AddParticle(ChunkMatrix *matrix, RGBA color, const Vec2f &position, uint16_t lifetime)
{
    if (!matrix) return;

    VoxelParticle *particle = new VoxelParticle(position, color);
    particle->particleLifeTime = lifetime+1;
    matrix->newParticles.push_back(particle);
}
