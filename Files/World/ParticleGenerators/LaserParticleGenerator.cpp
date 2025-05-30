#include "LaserParticleGenerator.h"

#include "Math/Vector.h"
#include "World/Chunk.h"
#include <cmath>
#include <iostream>

void Particle::LaserParticleGenerator::TickParticles()
{
    //get end position based on angle and length
    Vec2i currentPos = Vec2i(position);
    Vec2i endPosition = Vec2i(
        position.getX() + length * cos(angle),
        position.getY() + length * sin(angle)
    );
    
    int dx = abs(endPosition.getX() - currentPos.getX()), sx = currentPos.getX() < endPosition.getX() ? 1 : -1;
    int dy = -abs(endPosition.getY() - currentPos.getY()), sy = currentPos.getY() < endPosition.getY() ? 1 : -1;
    int err = dx + dy;

    int totalNumberOfParticlesInLine = std::max(abs(endPosition.getX() - currentPos.getX()), abs(endPosition.getY() - currentPos.getY()));
    int particleCount = 0;
    while(true){
        particleCount++;
        // Particle generation ------
        int alphaOffset = 0;
        Volume::VoxelElement *voxel = matrix->VirtualGetAt_NoLoad(currentPos);
        if (voxel) {
            // If we hit a solid voxel, stop the laser
            if(voxel->GetState() == Volume::State::Solid)
                break;
            else if(voxel->GetState() == Volume::State::Liquid)
                alphaOffset = -60;
        }

        // random alpha value from 70 - 180
        int alpha = rand() % 111 + 70;
        if(particleCount > totalNumberOfParticlesInLine-10) {
            //make alpha between 70 and 100
            alpha = rand() % 31 + 70;
            alphaOffset += (totalNumberOfParticlesInLine - 10 - particleCount) * 4;
        }
        // 10% chance to increase lifetime by one simulation tick
        bool increaseLifetime = rand() % 10 == 0;
        Particle::AddParticle(matrix, RGBA(249, 56, 39, alpha+alphaOffset), currentPos, 1+increaseLifetime);
        // --------------------------

        if (currentPos == endPosition) break;

        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            currentPos.x(currentPos.getX() + sx);
        }
        if (e2 <= dx) {
            err += dx;
            currentPos.y(currentPos.getY() + sy);
        }
    }
}