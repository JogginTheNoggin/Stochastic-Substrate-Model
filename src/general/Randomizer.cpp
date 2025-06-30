#include "../headers/util/Randomizer.h" // Assuming header is in the same directory or path is set



// TODO add getInt and other types without need to specify range, using data type min and max, for example unsigned int etc

int Randomizer::getInt(int min, int max) {
    // Purpose: Generate a random integer in [min, max].
    // Parameters: min, max - The range boundaries.
    // Return: A random integer.
    // Key Logic: depends on randomness source
    if (min > max) {
        // Or throw std::invalid_argument
        std::swap(min, max);
    }
    return source->getInt(min, max);
}

double Randomizer::getDouble(double min, double max) {
    // Purpose: Generate a random double in [min, max).
    // Parameters: min, max - The range boundaries.
    // Return: A random double.
    // Key Logic: uses source to get the double value
    if (min > max) {
        // Or throw std::invalid_argument
        std::swap(min, max);
    }
    return source->getDouble(min, max);
}

float Randomizer::getFloat(float min, float max){
    return source->getFloat(min, max); 
}
