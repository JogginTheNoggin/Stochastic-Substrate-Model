#pragma once

#include "gtest/gtest.h"
#include "controllers/TimeController.h"
#include "Payload.h"

/**
 * @class MockTimeController
 * @brief Mock implementation of TimeController for unit testing the Simulator.
 */
class MockTimeController : public TimeController {
public:
    // Enum to track the last significant method called.
    enum class LastCall {
        NONE,
        PROCESS_CURRENT_STEP,
        ADVANCE_STEP,
        ADD_TO_NEXT_STEP,
        DELIVER_AND_FLAG
    };

    // --- Public members for test inspection ---
    LastCall lastCall = LastCall::NONE;
    int callCount = 0;
    long long stepToReturn = 0;
    size_t activePayloadsToReturn = 5;

    /**
     * @brief Constructor requires a MetaController reference, just like the base class.
     * @param metaController A reference to a real or mock MetaController.
     */
    MockTimeController(MetaController& metaController) : TimeController(metaController) {}

    void reset() {
        lastCall = LastCall::NONE;
        callCount = 0;
        stepToReturn = 0;
        activePayloadsToReturn = 5; // Reset to default for each test.
    }

    // --- Overridden virtual methods ---
    void processCurrentStep() override {
        callCount++;
        lastCall = LastCall::PROCESS_CURRENT_STEP;
        
    }

    void advanceStep() override {
        callCount++;
        lastCall = LastCall::ADVANCE_STEP;
        stepToReturn++; // Simulate step advancement
        if (activePayloadsToReturn > 0) {
            activePayloadsToReturn--;
        }
    }

    void addToNextStepPayloads(const Payload& payload) override {
        callCount++;
        lastCall = LastCall::ADD_TO_NEXT_STEP;
    }

    void deliverAndFlagOperator(int targetOperatorId, int messageData) override {
        callCount++;
        lastCall = LastCall::DELIVER_AND_FLAG;
    }

    long long getCurrentStep() const override {
        return stepToReturn;
    }

    size_t getActivePayloadCount() const override {
        return activePayloadsToReturn;
    }

    

    bool saveState(const std::string& filePath) const override { return true; /* Simulate success */ }
    bool loadState(const std::string& filePath) override { return true; /* Simulate success */ }
    void printCurrentPayloads(std::ostream& out, bool pretty = false) const override { /* No-op */ }
};
