// In headers/util/RandomSource.h
#pragma once

#include <cstdint>

/**
 * @class RandomSource
 * @brief An abstract interface for different random number generation strategies.
 * @details This allows the main Randomizer class to use different sources
 * (e.g., pseudo-random or cryptographically secure) polymorphically.
 */
class RandomSource {
public:
    virtual ~RandomSource() = default;
    virtual int getInt(int min, int max) = 0;
    virtual double getDouble(double min, double max) = 0;
    virtual float getFloat(float min, float max) = 0;
};