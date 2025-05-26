#pragma once

#include <cstdint>

#include "World/ParticleGenerator.h"

namespace Particle {
    class LaserParticleGenerator : public ParticleGenerator {
    public:
        LaserParticleGenerator() : ParticleGenerator() {};
        LaserParticleGenerator(ChunkMatrix* matrix) : ParticleGenerator(matrix) {};
        ~LaserParticleGenerator(){};

        void TickParticles() override;

        int length = 0;
        // Angle in radians
        float angle = 0.0f;
    };
}