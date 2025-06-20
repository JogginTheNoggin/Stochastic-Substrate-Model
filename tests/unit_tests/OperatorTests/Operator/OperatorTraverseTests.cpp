#include "gtest/gtest.h"
#include "headers/operators/AddOperator.h"
#include "headers/operators/Operator.h"
#include "headers/Payload.h"
#include "headers/Scheduler.h"
#include "headers/controllers/TimeController.h"    // For Scheduler setup
#include "headers/controllers/MetaController.h"    // For Scheduler setup
#include "headers/UpdateScheduler.h"   // For completeness, though not directly used by traverse
#include "headers/UpdateEvent.h"
#include <memory>
#include <vector>
#include <unordered_set>

// Fixture for Operator traverse tests
class OperatorTraverseTest : public ::testing::Test {
protected:
    std::unique_ptr<MetaController> metaController;
    std::unique_ptr<TimeController> timeController;
    // op will be created within each test or a test-specific setup

    // Helper to check if Scheduler is available, to avoid crashing tests if not.
    // This is a workaround for the fact that Scheduler::get() throws.
    // In a real scenario with mocks, this wouldn't be needed.
    bool isSchedulerReallyAvailable() {
        try {
            if (Scheduler::get()) return true; // Attempt to get, might throw
        } catch (const std::runtime_error& e) {
            // Handle or log exception if necessary, e.g. std::cerr << e.what() << std::endl;
            return false; // Not available if get() throws
        }
        return false; // Should not be reached if get() behaves as expected (returns instance or throws)
    }


    void SetUp() override {
        // It was found earlier that Scheduler might need active MetaController and TimeController.
        // Initialize them here.
        metaController = std::make_unique<MetaController>(); // Default constructor
        // TimeController's constructor expects MetaController& not MetaController*
        timeController = std::make_unique<TimeController>(*metaController);


        // The TimeController constructor should call Scheduler::CreateInstance.
        // No need to call Scheduler::CreateInstance(timeController.get()) separately here.
        // We just need to ensure any previous instances are cleared.
        // Note: Scheduler::isInstanceAvailable() doesn't exist as per previous findings.
        // Resetting first ensures we don't have lingering state if a previous test failed badly.
        Scheduler::ResetInstances(); // Clear any old instances
        UpdateScheduler::ResetInstances(); // Clear any old UpdateScheduler instances

        // Now, after TimeController construction (which should handle Scheduler init),
        // we can create the Scheduler instance explicitly if TimeController does not.
        // Based on Scheduler.h, it has CreateInstance, not done by TimeController.
        // Based on TimeController.cpp, it does not create Scheduler instances itself.
        // Based on Scheduler.cpp, CreateInstance needs a TimeController*.
        // Scheduler::get() will throw if no instance.
        // So, we must call CreateInstance.
        if (timeController) {
             Scheduler::CreateInstance(timeController.get());
        }
        // Similarly for UpdateScheduler if it had a similar dependency, but it takes UpdateController.
        // For now, just ensuring it's reset.
    }

    void TearDown() override {
        // Reset scheduler states
        // Again, isInstanceAvailable() doesn't exist. ResetInstances handles cleanup.
        Scheduler::ResetInstances();
        UpdateScheduler::ResetInstances();
        // Controllers will be destructed automatically by unique_ptr.
    }
};

// Test Case 1: Payload distanceTraveled matches a connection bucket, last bucket
TEST_F(OperatorTraverseTest, TraversePayloadReachesFinalDestination) {
    // Pre-condition for this test: Scheduler must be available.
    if (!isSchedulerReallyAvailable()) {
        GTEST_SKIP() << "Scheduler not available, skipping test that relies on it.";
        return;
    }
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(1);
    op->addConnectionInternal(100, 2); // Target 100, distance 2

    Payload payload(50, op->getId()); // Message 50, source opId 1
    payload.distanceTraveled = 2;      // Set distance to match the bucket

    ASSERT_TRUE(payload.active);
    ASSERT_EQ(payload.distanceTraveled, 2);

    op->traverse(&payload);

    // In Operator::traverse:
    // 1. targetIdsPtr = outputConnections.get(payload->distanceTraveled) -> get(2) is found.
    // 2. Scheduler::get()->scheduleMessage(100, 50) is called. (Difficult to directly verify without mock)
    // 3. payload->distanceTraveled (2) == outputConnections.maxIdx() (2) is true.
    // 4. payload->active = false.
    // 5. The 'else { payload->distanceTraveled++; }' is NOT executed.

    EXPECT_FALSE(payload.active);
    EXPECT_EQ(payload.distanceTraveled, 2); // Distance should not change after being marked inactive this way.
                                            // The increment is in the 'else' block of `if (payload->distanceTraveled == outputConnections.maxIdx())`
}

// Test Case 2: Payload distanceTraveled has not yet reached any connection bucket
TEST_F(OperatorTraverseTest, TraversePayloadNotYetAtDestination) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(2);
    op->addConnectionInternal(200, 3); // Target 200, distance 3

    Payload payload(50, op->getId());
    payload.distanceTraveled = 1; // Current distance is 1, bucket is at 3

    op->traverse(&payload);

    // In Operator::traverse:
    // 1. targetIdsPtr = outputConnections.get(payload->distanceTraveled) -> get(1) is nullptr.
    // 2. The main 'if (targetIdsPtr != nullptr && !targetIdsPtr->empty())' is false.
    // 3. no change to payload.active and payload.distanceTraveled is set to 2 .

    EXPECT_TRUE(payload.active);
    EXPECT_EQ(payload.distanceTraveled, 2); // should have changed
}

// Test Case 3: Payload traverses past one bucket, heading to another (or end)
TEST_F(OperatorTraverseTest, TraversePayloadReachesIntermediateBucket) {
    if (!isSchedulerReallyAvailable()) {
        GTEST_SKIP() << "Scheduler not available, skipping test that relies on it.";
        return;
    }
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(3);
    op->addConnectionInternal(300, 1); // Bucket 1
    op->addConnectionInternal(301, 3); // Bucket 2 (maxIdx = 3)

    Payload payload(50, op->getId());
    payload.distanceTraveled = 1; // Matches first bucket

    op->traverse(&payload);

    // In Operator::traverse:
    // 1. targetIdsPtr = outputConnections.get(1) is found.
    // 2. scheduleMessage(300, 50) is called.
    // 3. payload->distanceTraveled (1) == outputConnections.maxIdx() (3) is false.
    // 4. else { payload->distanceTraveled++; } -> distanceTraveled becomes 2.
    // 5. Payload remains active.

    EXPECT_TRUE(payload.active);
    EXPECT_EQ(payload.distanceTraveled, 2); // Incremented past bucket 1
}


// Test Case 4: Inactive payload
TEST_F(OperatorTraverseTest, TraverseInactivePayload) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(4);
    op->addConnectionInternal(400, 1);

    Payload payload(50, op->getId());
    payload.distanceTraveled = 1;
    payload.active = false; // Mark as inactive

    op->traverse(&payload);

    // In Operator::traverse:
    // First check: `if (payload == nullptr || !payload->active || payload->distanceTraveled < 0)`
    // `!payload->active` is true, so it returns early.

    EXPECT_FALSE(payload.active); // Still inactive
    EXPECT_EQ(payload.distanceTraveled, 1); // Unchanged
}

// Test Case 5: Payload at max distance bucket (already tested by TraversePayloadReachesFinalDestination)
// This scenario is covered if maxIdx() is the *only* bucket.
// Let's refine final destination to make it clearer.
TEST_F(OperatorTraverseTest, TraversePayloadReachesSingleMaxDestination) {
    if (!isSchedulerReallyAvailable()) {
        GTEST_SKIP() << "Scheduler not available, skipping test that relies on it.";
        return;
    }
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(5);
    op->addConnectionInternal(500, 0); // Connection at distance 0, maxIdx = 0

    Payload payload(50, op->getId());
    payload.distanceTraveled = 0;

    op->traverse(&payload);
    // 1. targetIdsPtr = outputConnections.get(0) is found.
    // 2. scheduleMessage(500, 50) called.
    // 3. payload->distanceTraveled (0) == outputConnections.maxIdx() (0) is true.
    // 4. payload->active = false.
    EXPECT_FALSE(payload.active);
    EXPECT_EQ(payload.distanceTraveled, 0);
}


// Test Case 6: No connections on operator
TEST_F(OperatorTraverseTest, TraverseNoConnections) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(6);
    // No connections added

    Payload payload(50, op->getId());
    payload.distanceTraveled = 0;

    op->traverse(&payload);
    // 1. targetIdsPtr = outputConnections.get(0) is nullptr (maxIdx is -1).
    // 2. Main 'if' is false.
    // 3. payload should be come inactive, 4. distanceTraveled, not incremented

    EXPECT_FALSE(payload.active);
    EXPECT_EQ(payload.distanceTraveled, 0);
}

// Test Case 7: Payload distanceTraveled is past all connections
TEST_F(OperatorTraverseTest, TraversePayloadPastAllConnections) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(7);
    op->addConnectionInternal(700, 2); // Connection at distance 2 (maxIdx = 2)

    Payload payload(50, op->getId());
    payload.distanceTraveled = 3; // Payload is at distance 3

    op->traverse(&payload);
    // 1. targetIdsPtr = outputConnections.get(3) is nullptr.
    // 2. Main 'if' is false.
    // 3. set payload inactive, does not move forward

    EXPECT_FALSE(payload.active);
    EXPECT_EQ(payload.distanceTraveled, 3);
}

/* Not APPLICABLE, payload cannot be negative
// Test Case 8: Payload with negative distanceTraveled
TEST_F(OperatorTraverseTest, TraverseNegativePayloadDistance) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(8);
    op->addConnectionInternal(700, 1);
    Payload payload(50, op->getId());
    payload.distanceTraveled = -1; // Invalid distance (Doesn't work because payload is uint32_t), because very large
    payload.active = true;

    op->traverse(&payload);
    // In Operator::traverse:
    // First check: `if (payload == nullptr 
    // `payload->distanceTraveled < 0` is true, is set to inactive and returns early.

    EXPECT_FALSE(payload.active); // State should be set to false
    EXPECT_EQ(payload.distanceTraveled, -1);
}
*/ 

TEST_F(OperatorTraverseTest, TraverseWrongOpIdPayload) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(8); 

    Payload payload(50, 1); // passed opId is 1, expected is 8 
    payload.distanceTraveled = 0; // default distance
    payload.active = true;

    op->traverse(&payload);
    // In Operator::traverse:
    // operator ID is not same, does not process 

    EXPECT_TRUE(payload.active); // State should be unchanged by early exit
    EXPECT_EQ(payload.distanceTraveled, 0);
}

// Test Case 9: Null payload
TEST_F(OperatorTraverseTest, TraverseNullPayload) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(9);
    op->addConnectionInternal(900, 1);
    // Operator::traverse has `if (payload == nullptr ...)` check
    // We expect it to not crash and do nothing.
    ASSERT_NO_THROW(op->traverse(nullptr));
}
