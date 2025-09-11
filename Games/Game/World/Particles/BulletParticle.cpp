#include "BulletParticle.h"

#include <World/Particles/FallingParticle.h>
#include <World/ChunkMatrix.h>

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
    this->position = position;
}

Particle::BulletParticle::~BulletParticle()
{
}
bool Particle::BulletParticle::Step(ChunkMatrix* matrix){
    //new position variables
    this->position += m_dPosition;

    //Adjust position according to gravity
    m_dPosition = m_dPosition + Vec2f(0, Particle::GRAVITY * this->gravityMultiplier); // Apply gravity

    Vec2f futurePos = position + m_dPosition;

    //40% chance to create a falling particle in opposite direction
    if (rand() % 100 < 40)
    {
        VoxelParticle *particle = new FallingParticle(
            position,
            RGBA(240+variation(10), 240+variation(10), 240+variation(10), 100+variation(15)),
            std::atan2(m_dPosition.y, m_dPosition.x) + M_PI + variation(60)/100.0f,
            1.0f + variation(5)/10.0f,
            0.3f
        );
        particle->particleLifeTime = 40 + variation(20);
        matrix->AddParticle(particle);
    }

    // 20% chance to create a random red falling particle
    if (rand() % 100 < 20){
        VoxelParticle *particle = new FallingParticle(
            position,
            RGBA(240+variation(15), 90+variation(40), 2+variation(2), 205+variation(15)),
            rand() % 360 * M_PI / 180.0f,
            1.3f + variation(5)/10.0f,
            0.1f
        );
        particle->particleLifeTime = 35 + variation(25);
        matrix->AddParticle(particle);
    }

    Vec2f normalizedVelocity = m_dPosition / std::max(std::abs(m_dPosition.x), std::abs(m_dPosition.y));
    Vec2f stepPosition = position + normalizedVelocity;
    
    float futureDistanceSQRT = (futurePos - position).LengthSquared();
    float stepDistanceSQRT = (stepPosition - position).LengthSquared();

    while (stepDistanceSQRT < futureDistanceSQRT)
    {
        stepPosition += normalizedVelocity;
        stepDistanceSQRT = (stepPosition - position).LengthSquared();

        Volume::VoxelElement *stepVoxel = matrix->VirtualGetAt(stepPosition, true);
        if (!stepVoxel || stepVoxel->GetState() == Volume::State::Solid ||this->ShouldDie()){

            matrix->ExplodeAt(stepPosition, 2+(rand()%2-1));

            matrix->PlaceVoxelAt(stepPosition, "Iron", Volume::Temperature(100*this->damage), false, 1.0f, true, false);

            this->position = stepPosition;

            return true;
        }
    }
    
    if(!isTimeImmortal)
        this->particleLifeTime--;

    return false;
}

Vec2f Particle::BulletParticle::GetPosition() const {
    return Vec2f((Vec2i)this->position);
}