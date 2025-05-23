#pragma once

#include "World/Particle.h"

namespace Particle{
    constexpr float GRAVITY = 0.4f;

    class SolidFallingParticle : public Particle::VoxelParticle {
	private:
        Vec2f m_dPosition;
    public:
        Volume::VoxelElement *voxel;

        SolidFallingParticle();
        // Angle in radians
		SolidFallingParticle(Volume::VoxelElement *voxel, float angle, float speed);
        ~SolidFallingParticle();

		
		bool Step(ChunkMatrix* matrix) override;
        Vec2f GetPosition() const;
	};
    
    void AddSolidFallingParticle(ChunkMatrix *matrix, Volume::VoxelElement *voxel, float angle, float speed);
}