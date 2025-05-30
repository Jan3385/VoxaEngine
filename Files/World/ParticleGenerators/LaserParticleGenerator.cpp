#include "LaserParticleGenerator.h"

#include "Math/Vector.h"
#include "World/Chunk.h"
#include <cmath>
#include <iostream>

void Particle::LaserParticleGenerator::TickParticles()
{
    //get end position based on angle and length
    Vec2i startPos = Vec2i(position);
    Vec2i endPosition = Vec2i(
        position.getX() + length * cos(angle),
        position.getY() + length * sin(angle)
    );
    
    int dx = abs(endPosition.getX() - startPos.getX()), sx = startPos.getX() < endPosition.getX() ? 1 : -1;
    int dy = -abs(endPosition.getY() - startPos.getY()), sy = startPos.getY() < endPosition.getY() ? 1 : -1;
    int err = dx + dy;

    while(true){
        // Particle generation ------
        int alphaOffset = 0;
        Volume::VoxelElement *voxel = matrix->VirtualGetAt_NoLoad(startPos);
        if (voxel) {
            // If we hit a solid voxel, stop the laser
            if(voxel->GetState() == Volume::State::Solid)
                break;
            else if(voxel->GetState() == Volume::State::Liquid)
                alphaOffset = -60;
        }

        // random alpha value from 70 - 180
        int alpha = rand() % 111 + 70;
        // 10% chance to increase lifetime by one simulation tick
        bool increaseLifetime = rand() % 10 == 0;
        Particle::AddParticle(matrix, RGBA(249, 56, 39, alpha+alphaOffset), startPos, 1+increaseLifetime);
        // --------------------------

        if (startPos == endPosition) break;

        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            startPos.x(startPos.getX() + sx);
        }
        if (e2 <= dx) {
            err += dx;
            startPos.y(startPos.getY() + sy);
        }
    }
}