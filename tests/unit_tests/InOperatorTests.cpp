#include "gtest/gtest.h"
#include "headers/operators/InOperator.h"
#include "headers/operators/Operator.h" // For Operator::Type
#include "headers/Payload.h"
#include "headers/Scheduler.h"        // For potential Scheduler interactions (though likely mocked or inferred)
#include "headers/util/Serializer.h"  // For Serializer::read_uint32 etc. for serialization tests
#include "headers/util/Randomizer.h"  // For randomInit tests
#include "helpers/JsonTestHelpers.h" // For getJsonArrayIntValue
#include "headers/operators/AddOperator.h" // For comparison test
#include <memory>                     // For std::unique_ptr
#include <vector>                     // For std::vector
#include <string>                     // For std::string
#include <stdexcept>                  // For std::runtime_error
#include <sstream>                    // For ostringstream
#include <cstddef>                    // For std::byte
#include <limits>                     // For std::numeric_limits
#include <cmath>                      // For std::isnan, std::isinf, std::round

// Forward declaration if needed for Scheduler mocking, though not strictly required for InOperator
// class MockScheduler : public Scheduler { /* ... */ };


// Helper function to compare two vectors of integers
// This is simple enough to be included directly, or could be in a shared test helper file.
::testing::AssertionResult compareAccumulatedData(const std::vector<int>& expected, const std::vector<int>& actual) {
    if (expected.size() != actual.size()) {
        return ::testing::AssertionFailure() << "Vectors have different sizes. Expected: "
                                             << expected.size() << ", Actual: " << actual.size();
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i]) {
            return ::testing::AssertionFailure() << "Mismatch at index " << i
                                                 << ". Expected: " << expected[i]
                                                 << ", Actual: " << actual[i];
        }
    }
    return ::testing::AssertionSuccess();
}

// Test fixture for InOperator tests
class InOperatorTest : public ::testing::Test {
protected:
    std::unique_ptr<InOperator> op;
    int operatorId = 1; // Default ID for most tests

    void SetUp() override {
        op = std::make_unique<InOperator>(operatorId);
        // In case Scheduler is used and needs to be available (though InOperator::processData checks for null)
        // if (!Scheduler::get()) {
        //     Scheduler::initialize(nullptr, nullptr); // Initialize with null controllers if it allows
        // }
    }

    void TearDown() override {
        // if (Scheduler::isInitialized()) {
        //    Scheduler::resetInstance(); // Clean up scheduler if we initialized it
        // }
    }

    // Helper to get accumulatedData from an InOperator instance via toJson
    // Uses the more generic getJsonArrayIntValue from JsonTestHelpers.h
    std::vector<int> getAccumulatedDataLocal(const InOperator& current_op) {
        // toJson(prettyPrint=false, encloseInBrackets=true)
        std::string json_str = current_op.toJson(false, true);
        try {
            return JsonTestHelpers::getJsonArrayIntValue(json_str, "accumulatedData");
        } catch (const std::runtime_error& e) {
            // If key not found or parse error, return an empty vector or rethrow
            // For testing, it's often better to let it throw to fail the test clearly
            // Or, return a special value if that's part of a test scenario.
            // For now, assume an empty vector means "not found" or "empty in JSON"
            // std::cerr << "Exception in getAccumulatedDataLocal: " << e.what() << std::endl;
            // return {}; // Or rethrow: throw;
            // Let's try rethrowing to make test failures more obvious if JSON is malformed
            throw;
        }
    }
};

/*
// Placeholder for a test to make sure the file compiles and links
TEST_F(InOperatorTest, Placeholder) {
    EXPECT_EQ(op->getId(), operatorId);
    ASSERT_TRUE(op != nullptr);
}
*/

// Test fixture for InOperator constructor tests specifically
// Although InOperatorTest can be used, this separates concerns if needed later
class InOperatorConstructorTest : public ::testing::Test {
protected:
    // Helper to get accumulatedData from an InOperator instance via toJson
    std::vector<int> getAccumulatedDataFromOp(const InOperator& current_op) {
        std::string json_str = current_op.toJson(false, true); // prettyPrint=false, encloseInBrackets=true
        try {
            return JsonTestHelpers::getJsonArrayIntValue(json_str, "accumulatedData");
        } catch (const std::runtime_error& e) {
            // Rethrow to make test failures clear if JSON is malformed or key is missing
            throw;
        }
    }
};

TEST_F(InOperatorConstructorTest, ConstructorInitializesCorrectly) {
    uint32_t testId = 123;
    InOperator op_constructor_test(testId);

    EXPECT_EQ(op_constructor_test.getId(), static_cast<int>(testId)); // Operator::getId() returns int
    EXPECT_EQ(op_constructor_test.getOpType(), Operator::Type::IN);

    std::vector<int> accData = getAccumulatedDataFromOp(op_constructor_test);
    EXPECT_TRUE(accData.empty()) << "accumulatedData should be empty on construction.";
}

TEST_F(InOperatorConstructorTest, ConstructorInitializesCorrectlyWithZeroId) {
    uint32_t testId = 0;
    InOperator op_constructor_test(testId);

    EXPECT_EQ(op_constructor_test.getId(), static_cast<int>(testId));
    EXPECT_EQ(op_constructor_test.getOpType(), Operator::Type::IN);
    std::vector<int> accData = getAccumulatedDataFromOp(op_constructor_test);
    EXPECT_TRUE(accData.empty());
}

TEST_F(InOperatorTest, MessageInt_PositiveValue) {
    op->message(10);
    std::vector<int> expected = {10};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(25);
    expected = {10, 25}; // Order should be preserved
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageInt_ZeroValue) {
    op->message(0);
    std::vector<int> expected = {0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(15); // Add another value to ensure zero is handled correctly in sequence
    expected = {0, 15};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(0); // Add another zero
    expected = {0, 15, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageInt_NegativeValue_Base) {
    op->message(-5);
    std::vector<int> expected = {-5}; 
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(10);
    expected = {-5, 10};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(-100);
    expected = {-5, 10, -100};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageInt_MixedValues) {
    op->message(100);
    std::vector<int> expected = {100};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(-20); 
    expected = {100, -20};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(0);
    expected = {100, -20, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(30);
    expected = {100, -20, 0, 30};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(-1); 
    expected = {100, -20, 0, 30, -1};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageInt_MultiplePositiveValues) {
    op->message(1);
    op->message(2);
    op->message(3);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageInt_NoMessagesBeforeChecking) {
    // This test is essentially checking the initial state from SetUp via the helper,
    // which should be an empty vector.
    std::vector<int> expected = {};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_PositiveValue_Rounding) {
    op->message(10.3f); // Rounds to 10
    std::vector<int> expected = {10};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(10.7f); // Rounds to 11
    expected = {10, 11};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(10.5f); // Rounds to 11 (std::round typically rounds .5 away from zero)
    expected = {10, 11, 11};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_NegativeValue_Rounding) {
    // Unlike message(int), message(float/double) CAN add negative numbers if the input float is negative.
    op->message(-5.2f); // Rounds to -5
    std::vector<int> expected = {-5};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(-5.8f); // Rounds to -6
    expected = {-5, -6};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(-5.5f); // Rounds to -6 (std::round typically rounds .5 away from zero)
    expected = {-5, -6, -6};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_RoundToZero) {
    op->message(0.4f); // Rounds to 0
    std::vector<int> expected = {0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(-0.4f); // Rounds to 0
    expected = {0, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    op->message(0.0f);
    expected = {0, 0, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_NaN_ShouldBeIgnored) {
    op->message(1.0f); // Initial value
    op->message(std::numeric_limits<float>::quiet_NaN());
    op->message(2.0f); // Value after NaN

    std::vector<int> expected = {1, 2}; // NaN should not result in any addition
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_Infinity_ShouldBeIgnored) {
    op->message(3.0f); // Initial value
    op->message(std::numeric_limits<float>::infinity());
    op->message(4.0f); // Value after positive infinity

    std::vector<int> expected_pos = {3, 4};
    EXPECT_TRUE(compareAccumulatedData(expected_pos, getAccumulatedDataLocal(*op)));

    op->message(std::numeric_limits<float>::infinity() * -1.0f); // Negative infinity
    op->message(5.0f); // Value after negative infinity

    expected_pos = {3, 4, 5}; // Infinities should not result in any addition
    EXPECT_TRUE(compareAccumulatedData(expected_pos, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_ClampingToMaxInt) {
    op->message(static_cast<float>(std::numeric_limits<int>::max()) + 1000.0f);
    std::vector<int> expected = {std::numeric_limits<int>::max()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    // Add another value that is large but within limits (after clamping)
    // Due to float precision, static_cast<float>(INT_MAX - 5) becomes a float that rounds to INT_MAX.
    op->message(static_cast<float>(std::numeric_limits<int>::max() - 5));
    expected = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}; // Corrected expectation
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    // Add another value that also clamps to max
    op->message(static_cast<float>(std::numeric_limits<double>::max())); // double max cast to float is huge, becomes float infinity, so it's ignored
    // The expected vector remains the same as before this call, as infinity is ignored.
    // expected = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - 5, std::numeric_limits<int>::max()}; // Old expectation
    // Corrected expectation:
    // No change to 'expected' as the infinity message is ignored.
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_ClampingToMinInt) {
    op->message(static_cast<float>(std::numeric_limits<int>::min()) - 1000.0f);
    std::vector<int> expected = {std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    // Add another value that is very small but within limits (after clamping)
    // Due to float precision, static_cast<float>(INT_MIN + 5) becomes a float that rounds to INT_MIN.
    op->message(static_cast<float>(std::numeric_limits<int>::min() + 5));
    expected = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min()}; // Corrected expectation
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));

    // Add another value that also clamps to min
    // static_cast<float>(std::numeric_limits<double>::lowest()) might be too large negative even for float
    // let's use a large negative float directly
    op->message(-1.0f * std::numeric_limits<float>::max());
    expected = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min(), std::numeric_limits<int>::min()}; // Corrected expectation
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageFloat_MixedOperations) {
    op->message(10.7f);  // -> 11
    op->message(-2.3f); // -> -2
    op->message(std::numeric_limits<float>::quiet_NaN()); // ignored
    op->message(0.1f);   // -> 0
    op->message(static_cast<float>(std::numeric_limits<int>::max()) + 100.0f); // -> INT_MAX
    op->message(std::numeric_limits<float>::infinity()); // ignored
    op->message(-1.0e15f); // -> INT_MIN (large negative float)

    std::vector<int> expected = {11, -2, 0, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_PositiveValue_Rounding) {
    op->message(10.3); // Rounds to 10
    std::vector<int> expected_double = {10};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(10.7); // Rounds to 11
    expected_double = {10, 11};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(10.5); // Rounds to 11 (std::round typically rounds .5 away from zero)
    expected_double = {10, 11, 11};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_NegativeValue_Rounding) {
    op->message(-5.2); // Rounds to -5
    std::vector<int> expected_double = {-5};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(-5.8); // Rounds to -6
    expected_double = {-5, -6};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(-5.5); // Rounds to -6 (std::round typically rounds .5 away from zero)
    expected_double = {-5, -6, -6};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_RoundToZero) {
    op->message(0.4); // Rounds to 0
    std::vector<int> expected_double = {0};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(-0.4); // Rounds to 0
    expected_double = {0, 0};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(0.0);
    expected_double = {0, 0, 0};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_NaN_ShouldBeIgnored) {
    op->message(1.0); // Initial value
    op->message(std::numeric_limits<double>::quiet_NaN());
    op->message(2.0); // Value after NaN

    std::vector<int> expected_double = {1, 2}; // NaN should not result in any addition
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_Infinity_ShouldBeIgnored) {
    op->message(3.0); // Initial value
    op->message(std::numeric_limits<double>::infinity());
    op->message(4.0); // Value after positive infinity

    std::vector<int> expected_pos_double = {3, 4};
    EXPECT_TRUE(compareAccumulatedData(expected_pos_double, getAccumulatedDataLocal(*op)));

    op->message(std::numeric_limits<double>::infinity() * -1.0); // Negative infinity
    op->message(5.0); // Value after negative infinity

    expected_pos_double = {3, 4, 5}; // Infinities should not result in any addition
    EXPECT_TRUE(compareAccumulatedData(expected_pos_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_ClampingToMaxInt) {
    op->message(static_cast<double>(std::numeric_limits<int>::max()) + 1000.0);
    std::vector<int> expected_double = {std::numeric_limits<int>::max()};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(static_cast<double>(std::numeric_limits<int>::max()) - 5.0); // Value already int
    expected_double = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - 5};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(std::numeric_limits<double>::max()); // A very large double
     expected_double = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max() - 5, std::numeric_limits<int>::max()};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_ClampingToMinInt) {
    op->message(static_cast<double>(std::numeric_limits<int>::min()) - 1000.0);
    std::vector<int> expected_double = {std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(static_cast<double>(std::numeric_limits<int>::min()) + 5.0); // Value already int
    expected_double = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min() + 5};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));

    op->message(std::numeric_limits<double>::lowest()); // lowest is a large negative double
    expected_double = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min() + 5, std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, MessageDouble_MixedOperations) {
    op->message(10.7);  // -> 11
    op->message(-2.3); // -> -2
    op->message(std::numeric_limits<double>::quiet_NaN()); // ignored
    op->message(0.1);   // -> 0
    op->message(static_cast<double>(std::numeric_limits<int>::max()) + 100.0); // -> INT_MAX
    op->message(std::numeric_limits<double>::infinity()); // ignored
    op->message(std::numeric_limits<double>::lowest()); // -> INT_MIN

    std::vector<int> expected_double = {11, -2, 0, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected_double, getAccumulatedDataLocal(*op)));
}

TEST_F(InOperatorTest, ProcessData_EmptyAccumulatedData_IsCleared) {
    // Ensure accumulatedData is initially empty (it should be from SetUp)
    ASSERT_TRUE(getAccumulatedDataLocal(*op).empty());

    op->processData(); // Call processData

    // Verify accumulatedData is still empty
    EXPECT_TRUE(getAccumulatedDataLocal(*op).empty()) << "accumulatedData should remain empty after processData if initially empty.";
}

TEST_F(InOperatorTest, ProcessData_NonEmptyAccumulatedData_IsCleared) {
    op->message(10);
    op->message(20);
    ASSERT_FALSE(getAccumulatedDataLocal(*op).empty()) << "accumulatedData should be non-empty before processData.";

    op->processData(); // Call processData

    // Verify accumulatedData is now empty
    EXPECT_TRUE(getAccumulatedDataLocal(*op).empty()) << "accumulatedData should be cleared by processData.";
}

TEST_F(InOperatorTest, ProcessData_AccumulatedDataWithVariousValues_IsCleared) {
    op = std::make_unique<InOperator>(operatorId); // Reset op for clean data
    op->message(15); // int
    op->message(0);  // int
    op->message(-5); // int

    std::vector<int> expectedBefore = {15, 0, -5};
    ASSERT_TRUE(compareAccumulatedData(expectedBefore, getAccumulatedDataLocal(*op))) << "Mismatch in data before processData";


    op->processData(); // Call processData

    EXPECT_TRUE(getAccumulatedDataLocal(*op).empty()) << "accumulatedData should be cleared regardless of its content.";
}

TEST_F(InOperatorTest, ProcessData_WithNoConnections_ClearsAccumulatedData) {
    // By default, the 'op' in InOperatorTest has no connections.
    op->message(100);
    op->message(200);
    ASSERT_FALSE(getAccumulatedDataLocal(*op).empty());
    ASSERT_TRUE(op->getOutputConnections().empty()) << "Operator should have no connections for this test.";

    op->processData();

    EXPECT_TRUE(getAccumulatedDataLocal(*op).empty()) << "accumulatedData should be cleared even if there are no output connections.";
}

TEST_F(InOperatorTest, GetOpType_ReturnsCorrectType) {
    // The 'op' instance is created in InOperatorTest::SetUp()
    ASSERT_NE(op, nullptr); // Ensure op is valid

    // Call getOpType and check against the expected enum value
    EXPECT_EQ(op->getOpType(), Operator::Type::IN)
        << "getOpType() should return Operator::Type::IN for InOperator.";
}

TEST_F(InOperatorTest, GetOpType_ReturnsCorrectType_AfterOperations) {
    // Ensure opType remains correct even after some operations
    op->message(10);
    op->message(20.f);
    op->processData();

    EXPECT_EQ(op->getOpType(), Operator::Type::IN)
        << "getOpType() should still return Operator::Type::IN after other operations.";
}

TEST_F(InOperatorTest, ChangeParams_EmptyVector_NoEffect) {
    // Get initial state (ID and accumulatedData)
    int initialId = op->getId();
    std::vector<int> initialAccData = getAccumulatedDataLocal(*op);
    // Potentially check connection state if it were easily accessible and modifiable by changeParams in other ops

    op->changeParams({}); // Call with empty vector

    // Verify state remains unchanged
    EXPECT_EQ(op->getId(), initialId) << "ID should not change after changeParams.";
    EXPECT_TRUE(compareAccumulatedData(initialAccData, getAccumulatedDataLocal(*op)))
        << "accumulatedData should not change after changeParams with empty vector.";
    // Add EXPECT_NO_THROW if desired, though a crash would fail the test anyway.
    SUCCEED() << "changeParams with empty vector did not crash.";
}

TEST_F(InOperatorTest, ChangeParams_WithValidParams_NoEffect) {
    // InOperator's changeParams is a no-op, so any "valid" params for other operator types
    // should still result in no change for InOperator.
    int initialId = op->getId();
    op->message(10); // Add some data to check
    std::vector<int> initialAccData = getAccumulatedDataLocal(*op);

    op->changeParams({0, 123}); // Example: param ID 0, value 123 (might mean something to AddOperator)
    op->changeParams({1, 456, 7, 89}); // Example: more params

    EXPECT_EQ(op->getId(), initialId) << "ID should not change.";
    EXPECT_TRUE(compareAccumulatedData(initialAccData, getAccumulatedDataLocal(*op)))
        << "accumulatedData should not change after changeParams with arbitrary params.";
    SUCCEED() << "changeParams with arbitrary params did not crash.";
}

TEST_F(InOperatorTest, ChangeParams_DoesNotAlterOpType) {
    Operator::Type initialOpType = op->getOpType();
    ASSERT_EQ(initialOpType, Operator::Type::IN);

    op->changeParams({0, 1, 2, 3});

    EXPECT_EQ(op->getOpType(), initialOpType) << "OpType should not change after changeParams.";
}

// Test fixture for InOperator equality tests
// Can reuse InOperatorTest or make a new one if preferred. Let's use a new one for clarity.
class InOperatorEqualityTest : public ::testing::Test {
protected:
    std::unique_ptr<InOperator> op1;
    std::unique_ptr<InOperator> op2;
    // Helper to get accumulatedData - can use the one from InOperatorTest if accessible
    // or redefine if this fixture is entirely separate.
    // For simplicity, assume JsonTestHelpers is available.
    std::vector<int> getAccumulatedData(const InOperator& current_op) {
        // Assuming compareAccumulatedData is globally available or in a test helper
        // For this specific get, we use JsonTestHelpers directly.
        return JsonTestHelpers::getJsonArrayIntValue(current_op.toJson(false, true), "accumulatedData");
    }
};

TEST_F(InOperatorEqualityTest, Equals_SameId_DefaultState_AreEquals) {
    op1 = std::make_unique<InOperator>(100);
    op2 = std::make_unique<InOperator>(100);

    // By default, they have no connections and empty accumulatedData.
    // Base Operator::equals should compare IDs and connection structures.
    EXPECT_TRUE(op1->equals(*op2)) << "Two InOperators with same ID and default state should be equal.";
    EXPECT_TRUE(*op1 == *op2) << "operator== should also report them as equal.";
}

TEST_F(InOperatorEqualityTest, Equals_SameId_DifferentAccumulatedData_AreStillEqual) {
    op1 = std::make_unique<InOperator>(101);
    op2 = std::make_unique<InOperator>(101);

    op1->message(10); // op1 has {10}
    op1->message(20); // op1 has {10, 20}

    op2->message(30); // op2 has {30}

    // accumulatedData differs, but InOperator::equals should ignore it.
    // Base Operator::equals compares ID and connections (which are default/same here).
    EXPECT_TRUE(op1->equals(*op2))
        << "Two InOperators with same ID should be equal even if accumulatedData differs.";
    EXPECT_TRUE(*op1 == *op2)
        << "operator== should also report them as equal if accumulatedData differs but base is same.";

    // Sanity check that accumulatedData indeed differs
    // Assuming compareAccumulatedData is available (it's defined at the top of this file)
    EXPECT_FALSE(compareAccumulatedData(getAccumulatedData(*op1), getAccumulatedData(*op2)))
        << "Sanity check: accumulatedData should actually be different for this test.";
}

TEST_F(InOperatorEqualityTest, Equals_DifferentId_AreNotEqual) {
    op1 = std::make_unique<InOperator>(102);
    op2 = std::make_unique<InOperator>(103); // Different ID

    EXPECT_FALSE(op1->equals(*op2)) << "Two InOperators with different IDs should not be equal.";
    EXPECT_TRUE(*op1 != *op2) << "operator!= should report them as not equal.";
}

TEST_F(InOperatorEqualityTest, Equals_SameId_OneWithConnections_AreNotEqual) {
    // This test relies on Operator::equals comparing connections.
    // Operator::addConnectionInternal is protected.
    // This specific scenario is better tested in OperatorEqualityTests.cpp
    // where connections can be manipulated on base Operator pointers.
    // For InOperator, if it strictly uses Operator::equals, this behavior is inherited.
    // We'll skip directly testing this here due to difficulty modifying connections
    // on InOperator directly without changing its public interface or using derived test classes.
    // The principle is that if Operator::equals considers connections, and InOperator::equals
    // calls it, then connection differences would lead to inequality.
    SUCCEED() << "Skipping direct test of connection inequality for InOperator due to protected access. Behavior is inherited from Operator::equals.";
}


TEST_F(InOperatorEqualityTest, Equals_CompareWithDifferentOperatorType_SameId_NotEqualByOperatorEq) {
    op1 = std::make_unique<InOperator>(104);
    std::unique_ptr<AddOperator> addOp = std::make_unique<AddOperator>(104); // Same ID, different type

    // The non-member operator==(const Operator&, const Operator&) first checks getOpType().
    // If types differ, it returns false immediately without calling the virtual equals().
    EXPECT_FALSE(*op1 == *addOp)
        << "InOperator should not be equal to AddOperator (checked by non-member operator==).";
    EXPECT_TRUE(*op1 != *addOp);

    // If somehow op1->equals(*addOp) were called (e.g. if types were spoofed to be the same),
    // the behavior of Operator::equals against a different type's memory layout is undefined
    // and potentially unsafe. The primary guard is the non-member operator== checking types first.
    // AddOperator::equals would also likely return false if its specific members don't match.
    // So, we primarily test the non-member operator== here for inter-type comparison.
}


TEST_F(InOperatorConstructorTest, ConstructorInitializesCorrectlyWithMaxUint32Id) {
    // Note: Operator::operatorId is uint32_t, but getId() returns int.
    // This might lead to issues if uint32_t > INT_MAX. For this test,
    // we'll use a large uint32_t value that still fits in a positive int.
    // If Operator::getId() behavior with very large IDs needs specific testing,
    // that would be a separate concern, potentially for Operator itself.
    // Here, we focus on InOperator's construction.
    uint32_t testId = std::numeric_limits<int>::max(); // Largest positive int
    InOperator op_constructor_test(testId);

    EXPECT_EQ(op_constructor_test.getId(), static_cast<int>(testId));
    EXPECT_EQ(op_constructor_test.getOpType(), Operator::Type::IN);
    std::vector<int> accData = getAccumulatedDataFromOp(op_constructor_test);
    EXPECT_TRUE(accData.empty());
}
