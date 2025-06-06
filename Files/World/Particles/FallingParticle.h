#pragma once

#include "World/Particle.h"

namespace Particle{
    class FallingParticle : public Particle::VoxelParticle {
	private:
        Vec2f m_dPosition;
        RGBA color;
        float gravityMultiplier = 1.0f;
    public:

        FallingParticle();
        // Angle in radians
		FallingParticle(Vec2f position, RGBA color, float angle, float speed, float gravityMultiplier);
        ~FallingParticle();
		
		bool Step(ChunkMatrix* matrix) override;
        Vec2f GetPosition() const;
	};
    
    Particle::FallingParticle* AddFallingParticle(ChunkMatrix *matrix, RGBA color, 
        float angle, float speed, float gravityMultiplier,
        Vec2f position, float particleLifetime);
}