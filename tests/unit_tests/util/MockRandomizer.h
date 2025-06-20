#pragma once
#include "headers/util/Randomizer.h"
#include <vector>

// Helper for randomInit tests - a simple mock Randomizer
class MockRandomizer : public Randomizer {
private:
    std::vector<int> intValues;
    size_t intIndex = 0;
    std::vector<uint32_t> uint32Values;
    size_t uint32Index = 0;

public:
    MockRandomizer(std::vector<int> intVals = {}, std::vector<uint32_t> uint32Vals = {})
        : intValues(std::move(intVals)), uint32Values(std::move(uint32Vals)) {}

    int getInt(int min, int max) override {
        if (intIndex < intValues.size()) {
            return intValues[intIndex++];
        }
        // Fallback if not enough mock values provided, return min or a fixed value
        return min;
    }


    float getFloat(float min, float max) override {
        return min; // Not used by InOperator::randomInit
    }

    // Add more overrides if InOperator starts using other Randomizer methods
};