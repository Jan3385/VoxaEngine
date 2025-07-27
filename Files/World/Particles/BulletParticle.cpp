#include "World/Particles/BulletParticle.h"
#include "World/Particles/FallingParticle.h"
#include "World/ChunkMatrix.h"
#include "BulletParticle.h"
#include "Math/Temperature.h"

#include <cmath>

/// @brief Returns a random variation value.
/// @param max The maximum variation value.
/// @return A random variation value in the range [-max, max].
int variation(int max){
    return rand() % (2 * max + 1) - max;
}

using namespace Particle;
Particle::BulletParticle::BulletParticle()
{
    this->particleLifeTime = 600; 
    this->color = RGBA(255, 255, 0, 255);
    this->m_dPosition = Vec2f(0, 0);
    this->isTimeImmortal = false;
}

Particle::BulletParticle::BulletParticle(Vec2f position, float angle, float speed, float damage) :
    Particle::VoxelParticle(),
    m_dPosition(speed * std::cos(angle), speed * std::sin(angle)),
    damage(damage)
{
    this->particleLifeTime = 600; 
    this->color = RGBA(255, 255, 0, 255);
    this->fPosition = position;
}

Particle::BulletParticle::~BulletParticle()
{
}
bool Particle::BulletParticle::Step(ChunkMatrix* matrix){
    //new position variables
    this->fPosition += m_dPosition;

    //Adjust position according to gravity
    m_dPosition = m_dPosition + Vec2f(0, Particle::GRAVITY * this->gravityMultiplier); // Apply gravity

    Vec2f futurePos = fPosition + m_dPosition;

    //40% chance to create a falling particle in opposite direction
    if (rand() % 100 < 40)
    {
        Particle::AddFallingParticle(matrix, 
            RGBA(240+variation(10), 240+variation(10), 240+variation(10), 100+variation(15)),
            std::atan2(m_dPosition.y, m_dPosition.x) + M_PI + variation(60)/100.0f,
            1.0f + variation(5)/10.0f,
            0.3f,
            fPosition,
            40 + variation(20)
        );
    }

    // 20% chance to create a random red falling particle
    if (rand() % 100 < 20){
        Particle::AddFallingParticle(matrix, 
            RGBA(240+variation(15), 90+variation(40), 2+variation(2), 205+variation(15)),
            rand() % 360 * M_PI / 180.0f,
            1.3f + variation(5)/10.0f,
            0.1f,
            fPosition,
            35 + variation(25)
        );
    }

    Volume::VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos, true);
    if (!futureVoxel || futureVoxel->GetState() == Volume::State::Solid || this->ShouldDie())
    {
        //check if the position for the particle exists
        if (!matrix->IsValidWorldPosition(this->fPosition))
        {
            return true;
        }

        this->SetNextValidPosition(matrix);

        matrix->ExplodeAt(fPosition, 2+(rand()%2-1));
        
        matrix->PlaceVoxelAt(this->fPosition, "Iron", Volume::Temperature(100*this->damage), false, 1.0f, true, true);

        return true;
    }

    if(!isTimeImmortal)
        this->particleLifeTime--;

    return false;
}

Vec2f Particle::BulletParticle::GetPosition() const {
    return Vec2f((Vec2i)this->fPosition);
}

Particle::BulletParticle* Particle::AddBulletParticle(ChunkMatrix *matrix,
    float angle, float speed, float damage, Vec2f position)
{
    BulletParticle *particle = new BulletParticle(position, angle, speed, damage);
    matrix->newParticles.push_back(particle);

    return particle;
}

/// @brief Used to set the last valid position for the bullet particle if it were going in a straight line.
/// @param matrix The chunk matrix to check for valid positions.
void Particle::BulletParticle::SetNextValidPosition(ChunkMatrix *matrix){
    int iteration = 0;

    // Normalize the velocity vector
    m_dPosition = m_dPosition / std::max(std::abs(m_dPosition.x), std::abs(m_dPosition.y)); 

    Vec2f futurePos = fPosition + m_dPosition;
    Volume::VoxelElement *futureVoxel = matrix->VirtualGetAt(futurePos, true);

    while(futureVoxel && futureVoxel->GetState() != Volume::State::Solid && iteration < 5000)
    {
        // Move the particle in the direction of the velocity vector until we hit a solid voxel
        this->fPosition = futurePos;
        futurePos = fPosition + m_dPosition;
        futureVoxel = matrix->VirtualGetAt(futurePos, true);
        iteration++;
    }
}