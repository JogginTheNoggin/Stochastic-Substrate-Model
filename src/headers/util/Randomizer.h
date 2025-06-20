#pragma once

#include <random>
#include <cstdint> // For uint32_t etc.
#include <memory>  // For std::unique_ptr if managing engine dynamically

class Randomizer {
public:
    /**
     * @brief Default constructor. Seeds the pseudo-random engine with std::random_device.
     * This provides non-deterministic seeding for typical runs.
     */
    Randomizer();

    // TODO add cryptographically secure randominess support

    /**
     * @brief Constructor for reproducible runs. Seeds with a specific value.
     * @param seed The seed value for the pseudo-random number engine.
     */
    explicit Randomizer(unsigned int seed);

    /**
     * @brief Reseeds the pseudo-random number engine.
     * @param seed The new seed value.
     */
    void seed(unsigned int seed);

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

private:
    std::mt19937 pseudoRandomEngine; // Standard Mersenne Twister engine
    // std::random_device could be used here if you want a direct source of hardware entropy for some cases,
    // but it's typically used just to seed the engine.
};