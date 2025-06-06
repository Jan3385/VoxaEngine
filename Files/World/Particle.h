#pragma once

#include "World/Voxel.h"

class ChunkMatrix;

namespace Particle{
    constexpr float GRAVITY = 0.4f;
    class VoxelParticle{
	public:
		VoxelParticle();
		VoxelParticle(const Vec2f& position, RGBA color);
        virtual ~VoxelParticle() = default;

        RGBA color;

        // Particle lifetime in ticks
		uint16_t particleLifeTime = 1;
        // if particleLifeTime gets decreased or not
        bool isTimeImmortal = false;
		
        // Return true if particle should be removed
		virtual bool Step(ChunkMatrix* matrix) { 
            particleLifeTime--;
            return ShouldDie(); 
        };
        virtual Vec2f GetPosition() const { return fPosition; };
    protected:
        bool ShouldDie() const { return particleLifeTime <= 0 && !isTimeImmortal; };
        Vec2f fPosition;
	};

    void AddParticle(ChunkMatrix *matrix, RGBA color, const Vec2f& position, uint16_t lifetime);
}