#pragma once

#include "World/Particle.h"

namespace Particle{
    class SolidFallingParticle : public Particle::VoxelParticle {
	private:
        Vec2f m_dPosition;
    public:
        Volume::VoxelElement *voxel;

        SolidFallingParticle();
        // Angle in radians
		SolidFallingParticle(Volume::VoxelElement *voxel, float angle, float speed);
        ~SolidFallingParticle();

        bool precision = false;

		
		bool Step(ChunkMatrix* matrix) override;
        void SetNextValidPosition(ChunkMatrix *matrix);
        Vec2f GetPosition() const;
	};
    
    Particle::SolidFallingParticle *AddSolidFallingParticle(ChunkMatrix *matrix, Volume::VoxelElement *voxel, float angle, float speed, bool precision = false);
}