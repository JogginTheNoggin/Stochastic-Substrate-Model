#include "gtest/gtest.h"
#include "helpers/JsonTestHelpers.h" // For getJsonArrayIntValue and readGoldenFile
#include "headers/operators/InOperator.h"
#include "headers/operators/Operator.h" // For Operator::Type
#include "headers/Payload.h"
#include "headers/Scheduler.h"        // For potential Scheduler interactions
#include "headers/util/Serializer.h"  // For Serializer for serialization tests
#include "headers/util/Randomizer.h"  // For randomInit tests (if added later)
#include "headers/operators/AddOperator.h" // For comparison test
#include <memory>                     // For std::unique_ptr
#include <vector>                     // For std::vector
#include <string>                     // For std::string
#include <stdexcept>                  // For std::runtime_error
#include <sstream>                    // For ostringstream
#include <cstddef>                    // For std::byte
#include <limits>                     // For std::numeric_limits
#include <cmath>                      // For std::isnan, std::isinf, std::round

// Define the golden file directory relative to where the tests are run (build folder)
namespace {
    const std::string GOLDEN_FILES_DIR = "../tests/unit_tests/golden_files/inOperator/";
}

// Helper function to compare two vectors of integers
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
    uint32_t operatorId = 1; // Default ID for most tests

    void SetUp() override {
        op = std::make_unique<InOperator>(operatorId);
    }

    // Helper to get accumulatedData from an InOperator instance via toJson
    // Uses the more generic getJsonArrayIntValue from JsonTestHelpers.h
    // toJson(prettyPrint=false, encloseInBrackets=true) for consistency
    std::vector<int> getAccumulatedDataDirect(const InOperator& current_op) {
        std::string json_str = current_op.toJson(false, true);
        try {
            return JsonTestHelpers::getJsonArrayIntValue(json_str, "accumulatedData");
        } catch (const std::runtime_error& e) {
            throw; // Rethrow to fail test clearly
        }
    }
};

class InOperatorConstructorTest : public ::testing::Test {
    // No common setup needed for these static-like tests yet
};

TEST_F(InOperatorConstructorTest, ConstructorInitializesCorrectly) {
    uint32_t testId = 123;
    InOperator op_constructor_test(testId);

    EXPECT_EQ(op_constructor_test.getId(), static_cast<int>(testId));
    EXPECT_EQ(op_constructor_test.getOpType(), Operator::Type::IN);

    std::string actualJson = op_constructor_test.toJson(true, true);
    std::string expectedJsonFromFile = JsonTestHelpers::readGoldenFile(GOLDEN_FILES_DIR + "in_operator_constructor_id123.json");
    // toJson(prettyPrint=true, encloseInBrackets=true) for golden file comparison
    EXPECT_EQ(JsonTestHelpers::trim(actualJson), JsonTestHelpers::trim(expectedJsonFromFile));
}

TEST_F(InOperatorConstructorTest, ConstructorInitializesCorrectlyWithZeroId) {
    uint32_t testId = 0;
    InOperator op_constructor_test(testId);

    EXPECT_EQ(op_constructor_test.getId(), static_cast<int>(testId));
    EXPECT_EQ(op_constructor_test.getOpType(), Operator::Type::IN);

    std::string actualJson = op_constructor_test.toJson(true, true);
    std::string expectedJsonFromFile = JsonTestHelpers::readGoldenFile(GOLDEN_FILES_DIR + "in_operator_constructor_id0.json");
    EXPECT_EQ(JsonTestHelpers::trim(actualJson), JsonTestHelpers::trim(expectedJsonFromFile));
}

TEST_F(InOperatorConstructorTest, ConstructorInitializesCorrectlyWithMaxIntId) {
    uint32_t testId = static_cast<uint32_t>(std::numeric_limits<int>::max());
    InOperator op_constructor_test(testId);

    EXPECT_EQ(op_constructor_test.getId(), static_cast<int>(testId));
    EXPECT_EQ(op_constructor_test.getOpType(), Operator::Type::IN);

    std::string actualJson = op_constructor_test.toJson(true, true);
    std::string expectedJsonFromFile = JsonTestHelpers::readGoldenFile(GOLDEN_FILES_DIR + "in_operator_constructor_id_max_int.json");
    EXPECT_EQ(JsonTestHelpers::trim(actualJson), JsonTestHelpers::trim(expectedJsonFromFile));
}


// --- Message Tests ---
// These tests focus on the accumulatedData vector. Using getAccumulatedDataDirect
// which internally uses toJson(false, true) and JsonTestHelpers::getJsonArrayIntValue.
// This is efficient for checking just this part of the state.

TEST_F(InOperatorTest, MessageInt_PositiveValue) {
    op->message(10);
    std::vector<int> expected = {10};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));

    op->message(25);
    expected = {10, 25};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageInt_ZeroValue) {
    op->message(0);
    std::vector<int> expected = {0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));

    op->message(15);
    expected = {0, 15};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));

    op->message(0);
    expected = {0, 15, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageInt_NegativeValue) {
    op->message(-5);
    std::vector<int> expected = {-5}; 
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));

    op->message(10);
    expected = {-5, 10};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));

    op->message(-100);
    expected = {-5, 10, -100};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageInt_MixedValues) {
    op->message(100);
    op->message(-20); 
    op->message(0);
    op->message(30);
    op->message(-1); 
    std::vector<int> expected = {100, -20, 0, 30, -1};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageInt_MultiplePositiveValues) {
    op->message(1);
    op->message(2);
    op->message(3);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageInt_NoMessagesBeforeChecking) {
    std::vector<int> expected = {};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}


TEST_F(InOperatorTest, MessageFloat_PositiveValue_Rounding) {
    op->message(10.3f); // Rounds to 10
    op->message(10.7f); // Rounds to 11
    op->message(10.5f); // Rounds to 11
    std::vector<int> expected = {10, 11, 11};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageFloat_NegativeValue_Rounding) {
    op->message(-5.2f); // Rounds to -5
    op->message(-5.8f); // Rounds to -6
    op->message(-5.5f); // Rounds to -6
    std::vector<int> expected = {-5, -6, -6};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageFloat_RoundToZero) {
    op->message(0.4f);
    op->message(-0.4f);
    op->message(0.0f);
    std::vector<int> expected = {0, 0, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageFloat_NaN_ShouldBeIgnored) {
    op->message(1.0f);
    op->message(std::numeric_limits<float>::quiet_NaN());
    op->message(2.0f);
    std::vector<int> expected = {1, 2};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageFloat_Infinity_ShouldBeIgnored) {
    op->message(3.0f);
    op->message(std::numeric_limits<float>::infinity());
    op->message(4.0f);
    op->message(std::numeric_limits<float>::infinity() * -1.0f);
    op->message(5.0f);
    std::vector<int> expected = {3, 4, 5};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageFloat_ClampingToMaxInt) {
    op->message(static_cast<float>(std::numeric_limits<int>::max()) + 1000.0f); // Clamps to INT_MAX
    // The next value demonstrates behavior with numbers close to INT_MAX with float precision.
    // (float)(INT_MAX - 5) might round up to INT_MAX due to precision.
    // Let's test a value that should clearly be INT_MAX and another that rounds to INT_MAX.
    op->message(static_cast<float>(std::numeric_limits<int>::max())); // Exact INT_MAX
    // std::numeric_limits<double>::max() cast to float becomes float infinity, which is ignored.
    op->message(static_cast<float>(std::numeric_limits<double>::max()));

    std::vector<int> expected = {std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}


TEST_F(InOperatorTest, MessageFloat_ClampingToMinInt) {
    op->message(static_cast<float>(std::numeric_limits<int>::min()) - 1000.0f); // Clamps to INT_MIN
    op->message(static_cast<float>(std::numeric_limits<int>::min()));      // Exact INT_MIN
    op->message(-1.0f * std::numeric_limits<float>::max());                 // Large negative float, clamps to INT_MIN

    std::vector<int> expected = {std::numeric_limits<int>::min(), std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageFloat_MixedOperations) {
    op->message(10.7f);  // -> 11
    op->message(-2.3f); // -> -2
    op->message(std::numeric_limits<float>::quiet_NaN()); // ignored
    op->message(0.1f);   // -> 0
    op->message(static_cast<float>(std::numeric_limits<int>::max()) + 100.0f); // -> INT_MAX
    op->message(std::numeric_limits<float>::infinity()); // ignored
    op->message(-1.0e15f); // -> INT_MIN (large negative float, well beyond int range)

    std::vector<int> expected = {11, -2, 0, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}


TEST_F(InOperatorTest, MessageDouble_PositiveValue_Rounding) {
    op->message(10.3);
    op->message(10.7);
    op->message(10.5);
    std::vector<int> expected = {10, 11, 11};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_NegativeValue_Rounding) {
    op->message(-5.2);
    op->message(-5.8);
    op->message(-5.5);
    std::vector<int> expected = {-5, -6, -6};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_RoundToZero) {
    op->message(0.4);
    op->message(-0.4);
    op->message(0.0);
    std::vector<int> expected = {0, 0, 0};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_NaN_ShouldBeIgnored) {
    op->message(1.0);
    op->message(std::numeric_limits<double>::quiet_NaN());
    op->message(2.0);
    std::vector<int> expected = {1, 2};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_Infinity_ShouldBeIgnored) {
    op->message(3.0);
    op->message(std::numeric_limits<double>::infinity());
    op->message(4.0);
    op->message(std::numeric_limits<double>::infinity() * -1.0);
    op->message(5.0);
    std::vector<int> expected = {3, 4, 5};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_ClampingToMaxInt) {
    op->message(static_cast<double>(std::numeric_limits<int>::max()) + 1000.0); // Clamps
    op->message(static_cast<double>(std::numeric_limits<int>::max()) - 5.0);   // Exact value
    op->message(std::numeric_limits<double>::max());                           // Clamps

    std::vector<int> expected = {
        std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max() - 5,
        std::numeric_limits<int>::max()
    };
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_ClampingToMinInt) {
    op->message(static_cast<double>(std::numeric_limits<int>::min()) - 1000.0); // Clamps
    op->message(static_cast<double>(std::numeric_limits<int>::min()) + 5.0);    // Exact value
    op->message(std::numeric_limits<double>::lowest());                        // Clamps

    std::vector<int> expected = {
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::min() + 5,
        std::numeric_limits<int>::min()
    };
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}

TEST_F(InOperatorTest, MessageDouble_MixedOperations) {
    op->message(10.7);
    op->message(-2.3);
    op->message(std::numeric_limits<double>::quiet_NaN());
    op->message(0.1);
    op->message(static_cast<double>(std::numeric_limits<int>::max()) + 100.0);
    op->message(std::numeric_limits<double>::infinity());
    op->message(std::numeric_limits<double>::lowest());

    std::vector<int> expected = {11, -2, 0, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    EXPECT_TRUE(compareAccumulatedData(expected, getAccumulatedDataDirect(*op)));
}


// --- ProcessData Tests ---
TEST_F(InOperatorTest, ProcessData_EmptyAccumulatedData_IsCleared) {
    ASSERT_TRUE(getAccumulatedDataDirect(*op).empty());
    op->processData();
    EXPECT_TRUE(getAccumulatedDataDirect(*op).empty());
}

TEST_F(InOperatorTest, ProcessData_NonEmptyAccumulatedData_IsCleared) {
    op->message(10);
    op->message(20);
    ASSERT_FALSE(getAccumulatedDataDirect(*op).empty());
    op->processData();
    EXPECT_TRUE(getAccumulatedDataDirect(*op).empty());
}

TEST_F(InOperatorTest, ProcessData_WithNoConnections_ClearsAccumulatedData) {
    op->message(100);
    op->message(200);
    ASSERT_TRUE(op->getOutputConnections().empty());
    op->processData();
    EXPECT_TRUE(getAccumulatedDataDirect(*op).empty());
}

// --- Other Method Tests ---
TEST_F(InOperatorTest, GetOpType_ReturnsCorrectType) {
    EXPECT_EQ(op->getOpType(), Operator::Type::IN);
    op->message(10);
    op->processData();
    EXPECT_EQ(op->getOpType(), Operator::Type::IN);
}

TEST_F(InOperatorTest, ChangeParams_NoEffect) {
    std::string jsonBefore = op->toJson(false, true);
    op->changeParams({});
    EXPECT_EQ(op->toJson(false, true), jsonBefore);

    op->changeParams({0, 123, 7, 89});
    EXPECT_EQ(op->toJson(false, true), jsonBefore); // Should still be the same
}

// --- Equality Tests ---
class InOperatorEqualityTest : public ::testing::Test {
protected:
    std::unique_ptr<InOperator> op1;
    std::unique_ptr<InOperator> op2;

    // Using the same helper as InOperatorTest for consistency if needed, though direct access is fine
    std::vector<int> getAccumulatedData(const InOperator& current_op) {
        std::string json_str = current_op.toJson(false, true);
        return JsonTestHelpers::getJsonArrayIntValue(json_str, "accumulatedData");
    }
};

TEST_F(InOperatorEqualityTest, Equals_SameId_DefaultState_AreEquals) {
    op1 = std::make_unique<InOperator>(100);
    op2 = std::make_unique<InOperator>(100);
    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}

TEST_F(InOperatorEqualityTest, Equals_SameId_DifferentAccumulatedData_AreStillEqual) {
    op1 = std::make_unique<InOperator>(101);
    op2 = std::make_unique<InOperator>(101);
    op1->message(10);
    op2->message(30);
    // accumulatedData differs, but InOperator::equals should ignore it as it calls base Operator::equals
    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
    EXPECT_FALSE(compareAccumulatedData(getAccumulatedData(*op1), getAccumulatedData(*op2)));
}

TEST_F(InOperatorEqualityTest, Equals_DifferentId_AreNotEqual) {
    op1 = std::make_unique<InOperator>(102);
    op2 = std::make_unique<InOperator>(103);
    EXPECT_FALSE(op1->equals(*op2));
    EXPECT_TRUE(*op1 != *op2);
}

TEST_F(InOperatorEqualityTest, Equals_CompareWithDifferentOperatorType_SameId_NotEqualByOperatorEq) {
    op1 = std::make_unique<InOperator>(104);
    std::unique_ptr<AddOperator> addOp = std::make_unique<AddOperator>(104);
    EXPECT_FALSE(*op1 == *addOp); // Non-member operator== checks type first
    EXPECT_TRUE(*op1 != *addOp);
}

// --- ToJson Tests ---
class InOperatorToJsonTest : public ::testing::Test {
protected:
    // Helper to create a golden file name
    std::string goldenFilePath(const std::string& name) {
        return GOLDEN_FILES_DIR + name;
    }
};

TEST_F(InOperatorToJsonTest, ToJson_DefaultConstructorState) {
    InOperator op_json_test(1); // Same as the default InOperatorTest op
    std::string expectedJson = JsonTestHelpers::readGoldenFile(goldenFilePath("in_operator_constructor_id1.json"));
    // We need to create this golden file: in_operator_constructor_id1.json
    // Content:
    // {
    //   "opType": "IN",
    //   "operatorId": 1,
    //   "outputDistanceBuckets": [],
    //   "accumulatedData": []
    // }
    std::string actualJson = op_json_test.toJson(true, true);
    std::string expectedJsonFromFile = JsonTestHelpers::readGoldenFile(goldenFilePath("in_operator_constructor_id1.json"));
    EXPECT_EQ(JsonTestHelpers::trim(actualJson), JsonTestHelpers::trim(expectedJsonFromFile));
}

TEST_F(InOperatorToJsonTest, ToJson_WithAccumulatedData) {
    InOperator op_json_test(2);
    op_json_test.message(10);
    op_json_test.message(-20);
    std::string actualJson = op_json_test.toJson(true, true);
    std::string expectedJsonFromFile = JsonTestHelpers::readGoldenFile(goldenFilePath("in_operator_id2_acc_10_neg20.json"));
    // We need to create this golden file: in_operator_id2_acc_10_neg20.json
    // Content:
    // {
    //   "opType": "IN",
    //   "operatorId": 2,
    //   "outputDistanceBuckets": [],
    //   "accumulatedData": [10, -20]
    // }
    EXPECT_EQ(JsonTestHelpers::trim(actualJson), JsonTestHelpers::trim(expectedJsonFromFile));
}

TEST_F(InOperatorToJsonTest, ToJson_WithConnectionsAndAccumulatedData) {
    InOperator op_json_test(3);
    // InOperator doesn't have a public addConnection. This test would be more for Operator's toJson.
    // However, if InOperator's toJson somehow changed how connections are printed (it doesn't),
    // this would be relevant. For now, base class handles connection JSON.
    // We'll test with accumulated data and assume connections would be there if added via base.
    op_json_test.message(5);
    std::string actualJson = op_json_test.toJson(true, true);
    std::string expectedJsonFromFile = JsonTestHelpers::readGoldenFile(goldenFilePath("in_operator_id3_acc_5_no_conn.json"));
    // We need to create this golden file: in_operator_id3_acc_5_no_conn.json
    // Content:
    // {
    //   "opType": "IN",
    //   "operatorId": 3,
    //   "outputDistanceBuckets": [],
    //   "accumulatedData": [5]
    // }
    EXPECT_EQ(JsonTestHelpers::trim(actualJson), JsonTestHelpers::trim(expectedJsonFromFile));
}


// --- Serialization Tests ---
// Basic tests for InOperator's own (minimal) serialization logic.
// More extensive tests for base Operator serialization are in OperatorSerializeTests.cpp
class InOperatorSerializationTest : public ::testing::Test {};

TEST_F(InOperatorSerializationTest, SerializeDeserialize_Basic) {
    uint32_t id = 77;
    InOperator originalOp(id);
    // Add a connection to make the test more robust
    originalOp.addConnectionInternal(100, 1);

    std::vector<std::byte> serializedData = originalOp.serializeToBytes();
    ASSERT_FALSE(serializedData.empty());

    const std::byte* current = serializedData.data();
    const std::byte* end = serializedData.data() + serializedData.size();

    // --- Start of Corrected Deserialization Logic ---

    // 1. Simulate reading the 4-byte total size prefix.
    uint32_t dataSize = Serializer::read_uint32(current, end);
    ASSERT_EQ(dataSize, serializedData.size() - 4);

    // 2. CRITICAL FIX: Simulate reading the 2-byte Operator::Type.
    //    This advances 'current' to the correct position for the constructor.
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current, end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::IN);
    
    // 'current' now points to the start of the operatorId.
    // The constructor expects the stream to end at the end of its data block.
    const std::byte* operator_data_end = serializedData.data() + 4 + dataSize;
    
    // 3. Call the constructor with the correctly positioned pointer.
    std::unique_ptr<InOperator> deserializedOp = std::make_unique<InOperator>(current, operator_data_end);
    
    // --- End of Corrected Deserialization Logic ---


    ASSERT_NE(deserializedOp, nullptr);
    
    // Use the equality operator for a comprehensive check.
    EXPECT_TRUE(originalOp == *deserializedOp);
    
    // Also, verify that the transient data was not deserialized.
    std::string json_deserialized = deserializedOp->toJson(false, true);
    std::vector<int> accData = JsonTestHelpers::getJsonArrayIntValue(json_deserialized, "accumulatedData");
    EXPECT_TRUE(accData.empty());

    // Ensure all bytes of the operator's data block were consumed.
    EXPECT_EQ(current, operator_data_end);
}

TEST_F(InOperatorSerializationTest, Deserialize_EmptyBuffer_Throws) {
    // Test deserializing constructor with insufficient data
    const std::byte* current = nullptr;
    const std::byte* end = nullptr;
    EXPECT_THROW(InOperator op(current, end), std::runtime_error);

    std::vector<std::byte> tiny_buffer = {std::byte{1}}; // Too small
    current = tiny_buffer.data();
    end = tiny_buffer.data() + tiny_buffer.size();
    EXPECT_THROW(InOperator op(current, end), std::runtime_error);
}