#pragma once

#include "Math/Vector.h"

class ChunkMatrix;

namespace Particle{
    class ParticleGenerator {
    public:
        ParticleGenerator();
        ParticleGenerator(ChunkMatrix* matrix);
        ~ParticleGenerator();

        virtual void TickParticles() = 0;

        Vec2f position = Vec2f(0, 0);
        bool enabled = true;
    protected:
        ChunkMatrix* matrix = nullptr;
    };
}