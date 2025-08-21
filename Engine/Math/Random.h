#include <random>

class Random
{
private:
    unsigned int seed;
    std::mt19937 generator;
public:
    Random();
    Random(int seed);
    ~Random();

    void SetSeed(unsigned int seed);

    float GetFloat(float min, float max);
    int GetInt(int min, int max);
    bool GetBool();

    // disable copy and move semantics
    Random(const Random&) = delete;
    Random& operator=(const Random&) = delete;
};
