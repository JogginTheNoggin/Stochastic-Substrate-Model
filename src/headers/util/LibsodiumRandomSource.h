// In a new file, e.g., general/LibsodiumRandomSource.cpp and headers/util/LibsodiumRandomSource.h
#include "RandomSource.h"
#include <sodium.h> // Libsodium header
#include <stdexcept>

class LibsodiumRandomSource : public RandomSource {
public:
    LibsodiumRandomSource() {
        if (sodium_init() < 0) {
            throw std::runtime_error("Failed to initialize libsodium!");
        }
    }

    int getInt(int min, int max) override {
        if (min > max) std::swap(min, max);
        // randombytes_uniform gives a number in [0, upper_bound - 1]
        uint32_t range = static_cast<uint32_t>(max) - min + 1;
        if (range == 0) return min; // Handle edge case where max = INT_MAX and min = INT_MIN
        return static_cast<int>(randombytes_uniform(range)) + min;
    }

    double getDouble(double min, double max) override {
        if (min > max) std::swap(min, max);
        // Generate 64 random bits and scale them to the range [0.0, 1.0)
        uint64_t random_bits = randombytes_random();
        double scale = static_cast<double>(random_bits) / static_cast<double>(UINT64_MAX);
        return min + scale * (max - min);
    }

    float getFloat(float min, float max) override {
        return 0.0; // TODO temporary need actually implement
    }
};