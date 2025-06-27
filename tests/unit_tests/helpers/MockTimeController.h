#pragma once

#include "gtest/gtest.h"
#include "controllers/TimeController.h"
#include "Payload.h"
#include <string>
#include <vector>

// Forward declare MetaController to avoid including its full header in this mock header
class MetaController;

/**
 * @class MockTimeController
 * @brief Mock implementation of TimeController for unit testing dependent classes like Simulator.
 * @details This mock overrides the virtual methods of TimeController to record calls and parameters,
 * and to provide controllable return values for state-querying methods. This allows for
 * deterministic testing of components that interact with the TimeController. It also provides
 * `base*()` methods to call the real base class implementation for targeted testing.
 */
class MockTimeController : public TimeController {
public:
    /**
     * @brief Enum to track the last significant method called for test verification.
     */
    enum class LastCall {
        NONE,
        PROCESS_CURRENT_STEP,
        ADVANCE_STEP,
        ADD_TO_NEXT_STEP,
        DELIVER_AND_FLAG,
        SAVE_STATE,
        LOAD_STATE
    };

    // --- Public members for test inspection ---
    LastCall lastCall = LastCall::NONE;
    int callCount = 0;
    
    // --- Mockable state/return values ---
    long long stepToReturn = 0;
    size_t activePayloadsToReturn = 0;

    // --- Stored parameters from last calls ---
    Payload lastScheduledPayload;
    int lastTargetOperatorId = -1;
    int lastMessageData = -1;
    std::string lastFilePath;

    /**
     * @brief Constructor requires a MetaController reference, just like the base class.
     * @param metaController A reference to a real or mock MetaController.
     */
    MockTimeController(MetaController& metaController) : TimeController(metaController) {}

    /**
     * @brief Resets all tracking variables and mock state to their default values.
     * @details Call this in the SetUp() of your test fixture to ensure test isolation.
     */
    void reset() {
        lastCall = LastCall::NONE;
        callCount = 0;
        stepToReturn = 0;
        activePayloadsToReturn = 0;
        lastScheduledPayload = Payload();
        lastTargetOperatorId = -1;
        lastMessageData = -1;
        lastFilePath.clear();
    }

    // --- Overridden virtual methods ---
    void processCurrentStep() override {
        callCount++;
        lastCall = LastCall::PROCESS_CURRENT_STEP;
    }

    void advanceStep() override {
        callCount++;
        lastCall = LastCall::ADVANCE_STEP;
        stepToReturn++; // Simulate step advancement for realistic testing
    }

    void addToNextStepPayloads(const Payload& payload) override {
        callCount++;
        lastCall = LastCall::ADD_TO_NEXT_STEP;
        lastScheduledPayload = payload;
    }

    void deliverAndFlagOperator(uint32_t targetOperatorId, int messageData) override {
        callCount++;
        lastCall = LastCall::DELIVER_AND_FLAG;
        lastTargetOperatorId = targetOperatorId;
        lastMessageData = messageData;
    }

    long long getCurrentStep() const override {
        return stepToReturn;
    }

    size_t getActivePayloadCount() const override {
        return activePayloadsToReturn;
    }

    bool saveState(const std::string& filePath) const override {
        // Can't modify members in const method, so use const_cast for mock tracking
        auto* nonConstThis = const_cast<MockTimeController*>(this);
        nonConstThis->callCount++;
        nonConstThis->lastCall = LastCall::SAVE_STATE;
        nonConstThis->lastFilePath = filePath;
        return true; // Simulate success
    }

    bool loadState(const std::string& filePath) override {
        callCount++;
        lastCall = LastCall::LOAD_STATE;
        lastFilePath = filePath;
        return true; // Simulate success
    }
    
    void printCurrentPayloads(std::ostream& out, bool pretty = false) const override { 
        // No-op for the mock, as we don't want tests to produce console output.
    }

    // --- Base Class Method Calls ---
    // These methods allow tests to call the REAL TimeController implementation,
    // bypassing the mock overrides above.

    //Protected

    std::vector<Payload> baseLoadPayloads(std::istream& in, uint64_t count){
        return TimeController::loadPayloads(in, count);
    }

   
    std::unordered_set<uint32_t> baseLoadOperatorsToProcess(std::istream& in, uint64_t count){
        return TimeController::loadOperatorsToProcess(in, count);
    }

	
    void baseSavePayloads(std::ostream& out, const std::vector<Payload>& payloads) const {
        TimeController::savePayloads(out, payloads);
    }

    void baseSaveOperatorsToProcess(std::ostream& out) const{
        TimeController::saveOperatorsToProcess(out);
    }


	void baseProcessOperatorChecks(){
        TimeController::processOperatorChecks();
    }

	void baseProcessPayloadTraversal(){
        TimeController::processPayloadTraversal();
    }

    //Public 

    void baseProcessCurrentStep() {
        TimeController::processCurrentStep();
    }

    void baseAdvanceStep() {
        TimeController::advanceStep();
    }

    void baseAddToNextStepPayloads(const Payload& payload) {
        TimeController::addToNextStepPayloads(payload);
    }

    void baseDeliverAndFlagOperator(uint32_t targetOperatorId, int messageData) {
        TimeController::deliverAndFlagOperator(targetOperatorId, messageData);
    }

    long long baseGetCurrentStep() const {
        return TimeController::getCurrentStep();
    }

    size_t baseGetActivePayloadCount() const {
        return TimeController::getActivePayloadCount();
    }

    bool baseSaveState(const std::string& filePath) const {
        return TimeController::saveState(filePath);
    }

    bool baseLoadState(const std::string& filePath) {
        return TimeController::loadState(filePath);
    }
    
    void basePrintCurrentPayloads(std::ostream& out, bool pretty = false) const { 
        TimeController::printCurrentPayloads(out, pretty);
    }
};