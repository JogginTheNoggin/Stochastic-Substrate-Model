#pragma once

#include "util/Randomizer.h"
#include <gtest/gtest.h>
#include <queue>
#include <stdexcept>

/**
 * @class MockRandomizer
 * @brief A mock implementation of the Randomizer class for deterministic testing.
 *
 * @details This class inherits from Randomizer  and overrides its virtual methods to return
 * predictable, pre-programmed values instead of actual random numbers. It uses an
 * internal queue to store the sequence of integer values that should be returned by
 * successive calls to getInt(). It also tracks the number of calls to its methods
 * for verification in tests.
 *
 * This is essential for unit testing components like Layer::randomInit, where the
 * behavior depends on a sequence of random choices. By controlling the "random"
 * output, we can test specific code paths and ensure the component behaves as expected.
 *
 * Usage in tests:
 * 1. Instantiate MockRandomizer.
 * 2. Use `setNextInt()` to enqueue the sequence of integer values the test expects
 * the code-under-test to request.
 * 3. Pass the MockRandomizer instance to the method being tested.
 * 4. Use `getIntCallCount()` and other assertions to verify that the code-under-test
 * behaved as expected.
 */
class MockRandomizer : public Randomizer {
public:
    /**
     * @brief Default constructor.
     */
    MockRandomizer(){}

    /**
     * @brief Adds an integer value to the back of the queue of values to be returned by getInt().
     * @param value The integer value to enqueue.
     */
    void setNextInt(int value) {
        intQueue.push(value);
    }

    /**
     * @brief Overrides the base class getInt method to return a predictable value. 
     * @param min The minimum value of the range (ignored by the mock).
     * @param max The maximum value of the range (ignored by the mock).
     * @return The next integer from the internal queue.
     * @details Increments the call count. If the queue is empty, it causes a test failure
     * and returns 0 to allow the test to continue and report other failures.
     */
    int getInt(int min, int max) override {
        intCallCount++;
        if (intQueue.empty()) {
            ADD_FAILURE() << "MockRandomizer::getInt() was called, but the queue of expected return values is empty. Ensure the test is set up with enough values using setNextInt().";
            return 0; // Return a default value to prevent crashes and allow other test failures to be reported.
        }
        int nextValue = intQueue.front();
        intQueue.pop();
        return nextValue;
    }

    /**
     * @brief Overrides the base class getDouble method. Not used by the provided tests. 
     * @param min Minimum value.
     * @param max Maximum value.
     * @return Returns the minimum value as a safe default.
     */
    double getDouble(double min, double max) override {
        // Not used by the provided tests, but must be implemented.
        // Returning a predictable value like 'min' is a safe default.
        return min;
    }

    /**
     * @brief Overrides the base class getFloat method. Not used by the provided tests. 
     * @param min Minimum value.
     * @param max Maximum value.
     * @return Returns the minimum value as a safe default.
     */
    float getFloat(float min, float max) override {
        // Not used by the provided tests, but must be implemented.
        return min;
    }

    /**
     * @brief Returns the total number of times getInt() has been called.
     * @return The call count.
     */
    int getIntCallCount() const {
        return intCallCount;
    }

private:
    std::queue<int> intQueue;
    int intCallCount = 0;
};