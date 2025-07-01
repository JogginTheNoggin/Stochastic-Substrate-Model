#pragma once

#include "Simulator.h"
#include <string>
#include <vector>
#include <future> 
#include <mutex> // Required for std::mutex and std::lock_guard

/**
 * @class MockSimulator
 * @brief A mock implementation of the Simulator class for deterministic testing of the CLI.
 * @details This class overrides the virtual methods of the Simulator to record which
 * methods were called and with what arguments. It is now fully thread-safe, using a
 * mutex to protect access to its internal state variables.
 */
class MockSimulator : public Simulator {
public:
    // Enum to track the last method called, for easy verification in tests.
    enum class LastCall {
        NONE,
        LOAD_CONFIG,
        SAVE_CONFIG,
        LOAD_STATE,
        SAVE_STATE,
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
        std::lock_guard<std::mutex> lock(mockMutex);
        lastCall = LastCall::NONE;
        lastPath = "";
        lastNumOperators = -1;
        lastNumSteps = -1;
        lastLogFrequency = -1;
        lastSubmittedText = "";
        stopRequested = false;
        callCount = 0;
        runPromise = std::promise<void>();
    }

    // --- Overridden Simulator Methods ---
    void loadConfiguration(const std::string& filePath) override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::LOAD_CONFIG;
        lastPath = filePath;
    }

    bool saveConfiguration(const std::string& filePath) const override {
        std::lock_guard<std::mutex> lock(mockMutex);
        // We need to cast away constness to modify mock state in a const method.
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::SAVE_CONFIG;
        nonConstThis->lastPath = filePath;
        return true;
    }

    void loadState(const std::string& filePath) override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::LOAD_STATE;
        lastPath = filePath;
    }
    
    void saveState(const std::string& filePath) const override {
        std::lock_guard<std::mutex> lock(mockMutex);
        // We need to cast away constness to modify mock state in a const method.
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::SAVE_STATE;
        nonConstThis->lastPath = filePath;
    }

    void createNewNetwork(int numOperators) override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::NEW_NETWORK;
        lastNumOperators = numOperators;
    }

    void run(int numSteps) override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::RUN_STEPS;
        lastNumSteps = numSteps;
        try {
            runPromise.set_value(); // Signal that run has been called
        } catch (const std::future_error& e) {
            // This is expected if 'run' is called more than once in a single test.
            // We can safely ignore the "promise already satisfied" error.
        }
    }

    void run() override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::RUN_INACTIVE;
        try {
            runPromise.set_value(); // Signal that run has been called
        } catch (const std::future_error& e) {
            // This is expected if 'run' is called more than once in a single test.
            // We can safely ignore the "promise already satisfied" error.
        }
    }

    void requestStop() override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::REQUEST_STOP;
        stopRequested = true;
    }

    void setLogFrequency(int newLogFreq) override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::SET_LOG_FREQ;
        lastLogFrequency = newLogFreq;
    }

    void submitText(const std::string& text) override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::SUBMIT_TEXT;
        lastSubmittedText = text;
    }

    std::string getOutput() override {
        std::lock_guard<std::mutex> lock(mockMutex);
        callCount++;
        lastCall = LastCall::GET_OUTPUT;
        return "[Mock Output]";
    }

    SimulationStatus getStatus() const override {
        std::lock_guard<std::mutex> lock(mockMutex);
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::GET_STATUS;
        return {100, 5, 2, 50, 3};
    }

    std::string getNetworkJson(bool prettyPrint = true) const override {
        std::lock_guard<std::mutex> lock(mockMutex);
        auto* nonConstThis = const_cast<MockSimulator*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::GET_JSON;
        return "{ \"mockNetwork\": true }";
    }

private:
    // mutable allows the mutex to be locked even in const methods.
    mutable std::mutex mockMutex;
};