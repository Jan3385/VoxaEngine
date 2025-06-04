#include "World/ParticleGenerator.h"

#include "World/ChunkMatrix.h"
#include <algorithm>

Particle::ParticleGenerator::ParticleGenerator()
{
    this->matrix = nullptr;
}

Particle::ParticleGenerator::ParticleGenerator(ChunkMatrix *matrix)
    : matrix(matrix)
{
    matrix->particleGenerators.push_back(this);
}

Particle::ParticleGenerator::~ParticleGenerator()
{
    if (matrix)
    {
        auto it = std::find(matrix->particleGenerators.begin(), matrix->particleGenerators.end(), this);
        if (it != matrix->particleGenerators.end())
        {
            matrix->particleGenerators.erase(it);
        }
    }
}
