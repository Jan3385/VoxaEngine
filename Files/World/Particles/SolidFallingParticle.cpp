#include "World/Particles/SolidFallingParticle.h"
#include "SolidFallingParticle.h"
#include "World/ChunkMatrix.h"

#include <cmath>
#include <algorithm>
#include <iostream>

using namespace Particle;

SolidFallingParticle::SolidFallingParticle()
    :VoxelParticle(),
    m_dPosition(0, 0),
    voxel(nullptr)
{
    this->particleLifeTime = 600;
}

SolidFallingParticle::SolidFallingParticle(Volume::VoxelElement *voxel, float angle, float speed)
    :VoxelParticle(voxel->position, voxel->color),
    m_dPosition(speed * cos(angle), speed * sin(angle)),
    voxel(voxel)
{
    this->particleLifeTime = 600;
}
Particle::SolidFallingParticle::~SolidFallingParticle()
{
    if (voxel)
    {
        delete voxel;
        voxel = nullptr;
    }
}
bool SolidFallingParticle::Step(ChunkMatrix *matrix)
{
    //new position variables
    this->fPosition += m_dPosition;

    //Adjust position according to gravity
    m_dPosition = m_dPosition + Vec2f(0, Particle::GRAVITY); // Apply gravity

    Vec2f futurePos = fPosition + m_dPosition;

    Volume::VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos);
    if (!futureVoxel || futureVoxel->GetState() == Volume::State::Solid || this->ShouldDie())
    {
    	//check if the position for the particle exists
    	if (!matrix->IsValidWorldPosition(this->fPosition))
    	{
    		return true;
    	}

        if(this->precision)
            this->SetNextValidPosition(matrix);

		matrix->PlaceVoxelAt(this->fPosition, voxel->id, voxel->temperature, false, voxel->amount, false);
        delete voxel;
        voxel = nullptr;

    	return true;
    }

    if(!isTimeImmortal)
        this->particleLifeTime--;

    return false;
}

void Particle::SolidFallingParticle::SetNextValidPosition(ChunkMatrix *matrix)
{
    int iteration = 0;

    m_dPosition = m_dPosition / std::max(std::abs(m_dPosition.getX()), std::abs(m_dPosition.getY())); // Normalize the velocity vector

    Vec2f futurePos = fPosition + m_dPosition;
    Volume::VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos);

    while(futureVoxel && futureVoxel->GetState() != Volume::State::Solid && iteration < 5000)
    {
        // Move the particle in the direction of the velocity vector until we hit a solid voxel
        this->fPosition = futurePos;
        futurePos = fPosition + m_dPosition;
        futureVoxel = matrix->VirtualGetAt(futurePos);
        iteration++;
    }
}

Vec2f Particle::SolidFallingParticle::GetPosition() const
{
    return Vec2f((Vec2i)this->fPosition);
}

// No need to delete voxel pointer, done automatically
Particle::SolidFallingParticle *Particle::AddSolidFallingParticle(ChunkMatrix *matrix, Volume::VoxelElement *voxel, float angle, float speed, bool precision)
{
    if (voxel == nullptr) return nullptr; // Check for null pointer

    SolidFallingParticle *particle = new SolidFallingParticle(voxel, angle, speed);
    particle->precision = precision;

    matrix->newParticles.push_back(particle);

    return particle;
}
