#pragma once

#include "World/ParticleGenerator.h"

namespace Particle {
    class LaserParticleGenerator : public ParticleGenerator {
    public:
        LaserParticleGenerator() : ParticleGenerator() {};
        LaserParticleGenerator(ChunkMatrix* matrix) : ParticleGenerator(matrix) {};
        virtual ~LaserParticleGenerator() = default;

        void TickParticles() override;

        int length = 0;
        // Angle in radians
        float angle = 0.0f;
    };
}