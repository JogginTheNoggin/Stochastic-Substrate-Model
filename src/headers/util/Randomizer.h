#pragma once
#include "RandomSource.h"
#include <random>
#include <cstdint> // For uint32_t etc.
#include <memory>  // For std::unique_ptr if managing engine dynamically

class Randomizer {
private:
    std::unique_ptr<RandomSource> source;

    // inaccessible
    Randomizer(){};

public:

    // Constructor now takes a RandomSource strategy
    explicit Randomizer(std::unique_ptr<RandomSource> src) : source(std::move(src)) {}


    

    /**
     * @brief Generates a random integer within a specified range [min, max] (inclusive).
     * Uses the internal pseudo-random engine and a uniform integer distribution.
     * @param min The minimum value of the range.
     * @param max The maximum value of the range.
     * @return A random integer within [min, max].
     */
    virtual int getInt(int min, int max);

    /**
     * @brief Generates a random double within a specified range [min, max) (inclusive of min, exclusive of max).
     * Uses the internal pseudo-random engine and a uniform real distribution.
     * @param min The minimum value of the range.
     * @param max The maximum value of the range.
     * @return A random double within [min, max).
     */
    virtual double getDouble(double min, double max);

    // If you still want to explore a more "cryptographically strong" option later,
    // you could add methods here. For now, focusing on PRNG for simulation.
    // For example:
    // uint32_t getSecureRandomUint32(); // Might use std::random_device directly or wrap a CSPRNG library

    virtual float getFloat(float min, float max);
};