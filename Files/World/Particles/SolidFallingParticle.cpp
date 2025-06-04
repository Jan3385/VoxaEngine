#include "World/Particles/SolidFallingParticle.h"
#include "SolidFallingParticle.h"
#include "World/ChunkMatrix.h"

#include <cmath>

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
    m_dPosition = m_dPosition + Vec2f(0, Particle::GRAVITY);

    Vec2i futurePos = Vec2i(
    	static_cast<int>(fPosition.getX() + m_dPosition.getX()), 
    	static_cast<int>(fPosition.getY() + m_dPosition.getY())
    );

    Volume::VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos);
    if (!futureVoxel || futureVoxel->GetState() == Volume::State::Solid || this->ShouldDie())
    {
    	//check if the position for the particle exists
    	if (!matrix->IsValidWorldPosition(this->fPosition))
    	{
    		return true;
    	}

		matrix->PlaceVoxelAt(this->fPosition, voxel->id, voxel->temperature, false, voxel->amount, false);
        delete voxel;
        voxel = nullptr;

    	return true;
    }

    if(!isTimeImmortal)
        this->particleLifeTime--;

    return false;
}

Vec2f Particle::SolidFallingParticle::GetPosition() const
{
    return Vec2f((Vec2i)this->fPosition);
}

void Particle::AddSolidFallingParticle(ChunkMatrix *matrix, Volume::VoxelElement *voxel, float angle, float speed)
{
    if (voxel == nullptr) return; // Check for null pointer

    matrix->newParticles.push_back(new SolidFallingParticle(voxel, angle, speed));
}
