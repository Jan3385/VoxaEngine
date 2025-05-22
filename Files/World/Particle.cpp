#include "World/Particle.h"
#include "Particle.h"

#include "World/Chunk.h"

using namespace Particle;

VoxelParticle::VoxelParticle()
{
    this->fPosition = Vec2f(0, 0);
    this->color = RGBA(255, 0, 255, 255);
}

VoxelParticle::VoxelParticle(const Vec2f &position, RGBA color)
{
    this->fPosition = position;
    this->color = color;
}
