#pragma once

#include <World/ParticleGenerator.h>

namespace Particle {
    class LaserGenerator : public ParticleGenerator {
    public:
        LaserGenerator() : ParticleGenerator() {};
        LaserGenerator(ChunkMatrix* matrix) : ParticleGenerator(matrix) {};
        virtual ~LaserGenerator() = default;

        void TickParticles() override;

        int length = 0;
        // Angle in radians
        float angle = 0.0f;
    };
}