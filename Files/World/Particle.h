#pragma once

#include "World/Voxel.h"

class ChunkMatrix;

namespace Particle{
    class VoxelParticle{
	public:
		VoxelParticle();
		VoxelParticle(const Vec2f& position, RGBA color);
        virtual ~VoxelParticle() = default;

        RGBA color;

        // Particle lifetime in ticks
		uint16_t particleLifeTime = 50;
        // if particleLifeTime gets decreased or not
        bool isTimeImmortal = false;
		
		virtual bool Step(ChunkMatrix* matrix) { return false; };
        virtual Vec2f GetPosition() const { return fPosition; };
    protected:
        bool ShouldDie() const { return particleLifeTime <= 0 && !isTimeImmortal; };
        Vec2f fPosition;
	};

    
}