#include "../headers/util/Randomizer.h" // Assuming header is in the same directory or path is set

Randomizer::Randomizer() {
    // Purpose: Default constructor, seeds with std::random_device.
    // Parameters: None.
    // Return: N/A.
    // Key Logic: Create a std::random_device, use it to generate a seed, then seed the pseudoRandomEngine.
    std::random_device rd;
    pseudoRandomEngine.seed(rd());
}

Randomizer::Randomizer(unsigned int seedValue) : pseudoRandomEngine(seedValue) {
    // Purpose: Constructor for reproducible runs, seeds with a specific value.
    // Parameters: seedValue - The seed for the engine.
    // Return: N/A.
    // Key Logic: Directly initialize/seed the pseudoRandomEngine with the provided seed.
}

void Randomizer::seed(unsigned int seedValue) {
    // Purpose: Reseed the pseudo-random number engine.
    // Parameters: seedValue - The new seed.
    // Return: Void.
    // Key Logic: Call the seed() method of the internal engine.
    pseudoRandomEngine.seed(seedValue);
}

// TODO add getInt and other types without need to specify range, using data type min and max, for example unsigned int etc

int Randomizer::getInt(int min, int max) {
    // Purpose: Generate a random integer in [min, max].
    // Parameters: min, max - The range boundaries.
    // Return: A random integer.
    // Key Logic: Create a std::uniform_int_distribution for the given range
    //            and use it with the pseudoRandomEngine.
    if (min > max) {
        // Or throw std::invalid_argument
        std::swap(min, max);
    }
    std::uniform_int_distribution<int> dist(min, max);
    return dist(pseudoRandomEngine);
}

double Randomizer::getDouble(double min, double max) {
    // Purpose: Generate a random double in [min, max).
    // Parameters: min, max - The range boundaries.
    // Return: A random double.
    // Key Logic: Create a std::uniform_real_distribution for the given range
    //            and use it with the pseudoRandomEngine.
    if (min > max) {
        // Or throw std::invalid_argument
        std::swap(min, max);
    }
    std::uniform_real_distribution<double> dist(min, max);
    return dist(pseudoRandomEngine);
}

// Implementation for getSecureRandomUint32() would go here if you add it.
// If using libsodium, you'd include its header and call its functions.
// For example (conceptual, requires libsodium linked):
/*
#include <sodium.h> // If using libsodium

uint32_t Randomizer::getSecureRandomUint32() {
    // Purpose: Generate a cryptographically secure 32-bit unsigned integer.
    // Parameters: None.
    // Return: A random uint32_t.
    // Key Logic: Call libsodium's randombytes_uniform or similar.
    //            Ensure libsodium is initialized (sodium_init()).
    if (sodium_init() < 0) {
        // Handle error: libsodium couldn't be initialized
        throw std::runtime_error("Failed to initialize libsodium");
    }
    return randombytes_random(); // Or randombytes_buf for a specific size
}
*/