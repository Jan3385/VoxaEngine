#include "Random.h"

Random::Random()
{
    this->seed = 12345;
    generator.seed(this->seed);
}
Random::Random(int seed)
{
    this->seed = seed;
    generator.seed(this->seed);
}
Random::~Random()
{

}
void Random::SetSeed(unsigned int seed)
{
    this->seed = seed;
    generator.seed(this->seed);
}

/// @brief 
/// @param min minimum value (inclusive)
/// @param max maximum value (exclusive)
/// @return 
float Random::GetFloat(float min, float max)
{
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator);
}

/// @brief 
/// @param min minimum value (inclusive) 
/// @param max maximum value (inclusive)
/// @return 
int Random::GetInt(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

bool Random::GetBool()
{
    std::bernoulli_distribution distribution(0.5);
    return distribution(generator);
}
