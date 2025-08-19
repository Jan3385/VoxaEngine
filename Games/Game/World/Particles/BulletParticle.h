#pragma once

#include <World/Particle.h>

namespace Particle{
    class BulletParticle : public Particle::VoxelParticle {
	private:
        Vec2f m_dPosition;
        float damage = 1.0f;
    public:
        float gravityMultiplier = 1.0f;
        BulletParticle();
        // Angle in radians
		BulletParticle(Vec2f position, float angle, float speed, float damage);
        ~BulletParticle();
		
		bool Step(ChunkMatrix* matrix) override;
        Vec2f GetPosition() const;
	};
    
    Particle::BulletParticle* AddBulletParticle(ChunkMatrix *matrix,
        float angle, float speed, float damage, Vec2f position);
}