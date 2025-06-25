#pragma once

#include "Simulator.h"
#include <string>
#include <vector>
#include <future> // Required for std::promise

/**
 * @class MockSimulator
 * @brief A mock implementation of the Simulator class for deterministic testing of the CLI.
 * @details This class overrides the virtual methods of the Simulator to record which
 * methods were called and with what arguments. It does not perform any actual simulation
 * logic. Its state can be inspected by unit tests to verify that the CLI is behaving correctly.
 */
class MockSimulator : public Simulator {
public:
    // Enum to track the last method called, for easy verification in tests.
    enum class LastCall {
        NONE,
        LOAD_CONFIG,
        SAVE_CONFIG,
        NEW_NETWORK,
        RUN_STEPS,
        RUN_INACTIVE,
        REQUEST_STOP,
        SET_LOG_FREQ,
        SUBMIT_TEXT,
        GET_OUTPUT,
        GET_STATUS,
        GET_JSON
    };


    

    // --- Public State for Test Inspection ---
    LastCall lastCall = LastCall::NONE;
    std::string lastPath;
    int lastNumOperators = -1;
    int lastNumSteps = -1;
    int lastLogFrequency = -1;
    std::string lastSubmittedText;
    bool stopRequested = false;
    int callCount = 0;

    // Promise to signal when a run method has been invoked
    std::promise<void> runPromise;


    /**
     * @brief Default constructor.
     * @details Calls the base Simulator constructor with an empty path, as no real controllers are needed.
     */
    MockSimulator() : Simulator("") {
        reset();
    }

    /**
     * @brief Virtual destructor.
     */
    virtual ~MockSimulator() override = default;

    /**
     * @brief Resets the mock's internal state for a new test case.
     */
    void reset() {
        lastCall = LastCall::NONE;
        lastPath = "";
        lastNumOperators = -1;
        lastNumSteps = -1;
        lastLogFrequency = -1;
        lastSubmittedText = "";
        stopRequested = false;
        callCount = 0;

        // Reset the promise for the next test that needs it
        runPromise = std::promise<void>();
    }

    // --- Overridden Simulator Methods ---
    void loadConfiguration(const std::string& filePath) override {
        callCount++;
        lastCall = LastCall::LOAD_CONFIG;
        lastPath = filePath;
    }

    void saveConfiguration(const std::string& filePath) const override {
        // We need to cast away constness to modify mock state in a const method.
        // This is a common pattern for mocking.
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::SAVE_CONFIG;
        nonConstThis->lastPath = filePath;
    }

    void createNewNetwork(int numOperators) override {
        callCount++;
        lastCall = LastCall::NEW_NETWORK;
        lastNumOperators = numOperators;
    }

    void run(int numSteps) override {
        callCount++;
        lastCall = LastCall::RUN_STEPS;
        lastNumSteps = numSteps;
        runPromise.set_value(); // Signal that run has been called
    }

    void run() override {
        callCount++;
        lastCall = LastCall::RUN_INACTIVE;
        runPromise.set_value(); // Signal that run has been called
    }

    void requestStop() override {
        callCount++;
        lastCall = LastCall::REQUEST_STOP;
        stopRequested = true;
    }

    void setLogFrequency(int newLogFreq) override {
        callCount++;
        lastCall = LastCall::SET_LOG_FREQ;
        lastLogFrequency = newLogFreq;
    }

    void submitText(const std::string& text) override {
        callCount++;
        lastCall = LastCall::SUBMIT_TEXT;
        lastSubmittedText = text;
    }

    std::string getOutput() override {
        callCount++;
        lastCall = LastCall::GET_OUTPUT;
        return "[Mock Output]";
    }

    SimulationStatus getStatus() const override {
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::GET_STATUS;
        // Return a fixed, predictable status for testing.
        return {100, 5, 2, 50, 3};
    }

    std::string getNetworkJson(bool prettyPrint = true) const override {
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::GET_JSON;
        return "{ \"mockNetwork\": true }";
    }
};