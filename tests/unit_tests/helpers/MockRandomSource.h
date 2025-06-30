// In a new file, e.g., general/PseudoRandomSource.cpp and headers/util/PseudoRandomSource.h
#include "util/RandomSource.h"
#include <random>

class MockRandomSource : public RandomSource {
;

public:

    /**
     * @brief for reproducing results of random value
     * @param seed the seed for producing random values
     */
    explicit MockRandomSource(unsigned int seed) {}

    MockRandomSource() {

    }

    
    /**
     * @brief Reseeds the pseudo-random number engine.
     * @param seed The new seed value.
     */
    void seed(unsigned int seed){

    }

    int getInt(int min, int max) override {
        if (min > max) std::swap(min, max);
        std::uniform_int_distribution<int> dist(min, max);
        return 1;
    }

    double getDouble(double min, double max) override {
        return 1.0;
    }

    // TODO temporary, does nothing
    float getFloat(float min, float max) override{
        return 0.0;
    }
};