// In a new file, e.g., general/PseudoRandomSource.cpp and headers/util/PseudoRandomSource.h
#include "RandomSource.h"
#include <random>

class PseudoRandomSource : public RandomSource {
private:
    std::mt19937 pseudoRandomEngine;

public:

    /**
     * @brief for reproducing results of random value
     * @param seed the seed for producing random values
     */
    explicit PseudoRandomSource(unsigned int seed) : pseudoRandomEngine(seed) {}

    PseudoRandomSource() {
        std::random_device rd;
        pseudoRandomEngine.seed(rd());
    }

    
    /**
     * @brief Reseeds the pseudo-random number engine.
     * @param seed The new seed value.
     */
    void seed(unsigned int seed){
        // Purpose: Reseed the pseudo-random number engine.
        // Parameters: seedValue - The new seed.
        // Return: Void.
        // Key Logic: Call the seed() method of the internal engine.
        pseudoRandomEngine.seed(seed); 

    }

    int getInt(int min, int max) override {
        if (min > max) std::swap(min, max);
        std::uniform_int_distribution<int> dist(min, max);
        return dist(pseudoRandomEngine);
    }

    double getDouble(double min, double max) override {
        if (min > max) std::swap(min, max);
        std::uniform_real_distribution<double> dist(min, max);
        return dist(pseudoRandomEngine);
    }

    // TODO temporary, does nothing
    float getFloat(float min, float max) override{
        return 0.0;
    }
};