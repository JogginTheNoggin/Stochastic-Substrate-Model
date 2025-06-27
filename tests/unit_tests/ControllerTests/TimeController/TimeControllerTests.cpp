#include "gtest/gtest.h"
#include "helpers/MockTimeController.h"
#include "helpers/MockMetaController.h"
#include "controllers/TimeController.h"
#include "controllers/MetaController.h"
#include "Scheduler.h"
#include "Payload.h"
#include <memory>
#include <vector>
#include <string>

// Test fixture for TimeController tests
class TimeControllerTest : public ::testing::Test {
protected:
    // Mocks to act as dependencies and observers
    std::unique_ptr<MockMetaController> mockMetaController;
    std::unique_ptr<MockTimeController> mockTimeController;

    // The path for persistence tests
    std::string tempStateFile = "time_controller_state.bin";

    void SetUp() override {
        // Instantiate dependencies for each test
        mockMetaController = std::make_unique<MockMetaController>();
        
        // The TimeController requires a MetaController. We pass our mock.
        // We use the MockTimeController to gain access to the base class methods.
        mockTimeController = std::make_unique<MockTimeController>(*mockMetaController);

        // The Scheduler needs to be initialized for some TimeController methods to work
        Scheduler::ResetInstances(); // Ensure a clean slate
        Scheduler::CreateInstance(mockTimeController.get());
        
        // Clean up any old state files before the test runs
        std::remove(tempStateFile.c_str());
    }

    void TearDown() override {
        // Destructors of unique_ptr will handle cleanup.
        // Reset the static scheduler instance after each test.
        Scheduler::ResetInstances();
        // Clean up state file after the test
        std::remove(tempStateFile.c_str());
    }
};

// --- Constructor and Initial State Tests ---

TEST_F(TimeControllerTest, ConstructorInitializesStateCorrectly) {
    // This test verifies the state after the constructor is called in SetUp.
    EXPECT_EQ(mockTimeController->baseGetCurrentStep(), 0);
    EXPECT_EQ(mockTimeController->baseGetActivePayloadCount(), 0);
    // The internal payload vectors and operator set should be empty by default.
}

// --- Core Logic Tests (`advanceStep`, `deliverAndFlagOperator`, etc.) ---

TEST_F(TimeControllerTest, AdvanceStepMovesPayloadsAndIncrementsStep) {
    // ARRANGE: Schedule a payload for the "next" step.
    Payload p1(100, 1);
    mockTimeController->baseAddToNextStepPayloads(p1);

    // Check initial state
    ASSERT_EQ(mockTimeController->baseGetCurrentStep(), 0);
    ASSERT_EQ(mockTimeController->baseGetActivePayloadCount(), 0);
    
    // ACT: Advance the simulation by one step.
    mockTimeController->baseAdvanceStep();

    // ASSERT: The step counter should increment, and the payload should now be "current".
    EXPECT_EQ(mockTimeController->baseGetCurrentStep(), 1);
    EXPECT_EQ(mockTimeController->baseGetActivePayloadCount(), 1);

    // ACT: Advance again with no new payloads scheduled.
    mockTimeController->baseAdvanceStep();
    
    // ASSERT: The current payloads from the previous step should be cleared.
    EXPECT_EQ(mockTimeController->baseGetCurrentStep(), 2);
    EXPECT_EQ(mockTimeController->baseGetActivePayloadCount(), 0);
}

TEST_F(TimeControllerTest, DeliverAndFlagOperatorFlagsOperatorForNextStep) {
    // ARRANGE: An operator ID and message data to deliver.
    int targetOpId = 5;
    int message = 123;

    // ACT 1: Deliver a message. This should call MetaController and flag the operator.
    mockTimeController->baseDeliverAndFlagOperator(targetOpId, message);

    // ASSERT 1: MetaController should have been called to deliver the message.
    EXPECT_EQ(mockMetaController->lastCall, MockMetaController::LastCall::MESSAGE_OP);
    EXPECT_EQ(mockMetaController->lastOperatorId, targetOpId);
    EXPECT_EQ(mockMetaController->lastMessageData, message);

    // ACT 2: Advance to the next step. This is crucial.
    mockTimeController->baseAdvanceStep();
    
    // ACT 3: Process the new step. This should now process the flagged operator.
    mockTimeController->baseProcessCurrentStep();

    // ASSERT 2: Now, processOpData for the flagged operator should have been called.
    EXPECT_EQ(mockMetaController->lastCall, MockMetaController::LastCall::PROCESS_OP_DATA);
    EXPECT_EQ(mockMetaController->lastOperatorId, targetOpId);
}

TEST_F(TimeControllerTest, ProcessCurrentStepCallsTraverseOnPayloads) {
    // ARRANGE: Add a payload to be processed in the current step.
    Payload p1(100, 1);
    mockTimeController->baseAddToNextStepPayloads(p1);
    mockTimeController->baseAdvanceStep(); // Move payload to current
    ASSERT_EQ(mockTimeController->baseGetActivePayloadCount(), 1);

    // ACT: Process the current step.
    mockTimeController->baseProcessCurrentStep();

    // ASSERT: The traversePayload method on MetaController should have been called.
    EXPECT_EQ(mockMetaController->lastCall, MockMetaController::LastCall::TRAVERSE_PAYLOAD);
}


// --- Persistence Tests (`saveState` and `loadState`) ---

TEST_F(TimeControllerTest, SaveAndLoadStateRoundTrip) {
    // ARRANGE: Set up a specific state in the first TimeController.
    Payload p_current(100, 1, 10, true);
    Payload p_next(200, 2, 20, true);
    int opId_to_process = 5;
    
    mockTimeController->baseAddToNextStepPayloads(p_next); // Add to next
    mockTimeController->baseAdvanceStep(); // Now p_next is current, currentStep is 1
    mockTimeController->baseAddToNextStepPayloads(p_current); // Add p_current, it will be in next
    mockTimeController->baseDeliverAndFlagOperator(opId_to_process, 99); // Flag an operator

    // Verify initial state before saving
    ASSERT_EQ(mockTimeController->baseGetCurrentStep(), 1);
    ASSERT_EQ(mockTimeController->baseGetActivePayloadCount(), 1);

    // ACT 1: Save the state to a file.
    bool saveResult = mockTimeController->baseSaveState(tempStateFile);
    ASSERT_TRUE(saveResult);

    // ARRANGE 2: Create a new TimeController to load the state into.
    auto newMetaController = std::make_unique<MockMetaController>();
    auto newTimeController = std::make_unique<MockTimeController>(*newMetaController);
    
    // ACT 2: Load the state from the file.
    bool loadResult = newTimeController->baseLoadState(tempStateFile);
    ASSERT_TRUE(loadResult);
    
    // ASSERT: The state of the new controller should match the saved state.
    // Note: Loading resets the step counter to 0, which is expected.
    EXPECT_EQ(newTimeController->baseGetCurrentStep(), 0);
    // It should have loaded 1 "current" payload and 1 "next" payload.
    // Let's check the active payload count after one advance step.
    EXPECT_EQ(newTimeController->baseGetActivePayloadCount(), 1);
    newTimeController->baseAdvanceStep();
    EXPECT_EQ(newTimeController->baseGetActivePayloadCount(), 1);

    // ASSERT 2: Check that the flagged operator was loaded correctly by processing a step.
    newTimeController->baseProcessCurrentStep();
    // Check the mock controller's state after processing.
    // The *last* call in processCurrentStep is traversePayload.
    EXPECT_EQ(newMetaController->lastCall, MockMetaController::LastCall::TRAVERSE_PAYLOAD);
    // However, the call to processOpData should have happened first, setting lastOperatorId.
    // The call to traversePayload does not change lastOperatorId, so we can check it.
    EXPECT_EQ(newMetaController->lastOperatorId, opId_to_process);
}

TEST_F(TimeControllerTest, LoadStateWithNonExistentFileReturnsFalse) {
    // ARRANGE: A file path that does not exist.
    std::string badPath = "non_existent_time_state.bin";
    std::remove(badPath.c_str()); // Ensure it's gone.

    // ACT & ASSERT: loadState should return false.
    EXPECT_FALSE(mockTimeController->baseLoadState(badPath));
}