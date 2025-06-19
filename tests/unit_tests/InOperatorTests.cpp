#include "gtest/gtest.h"
#include "headers/operators/InOperator.h"
#include "headers/operators/AddOperator.h" // For equality tests against different types
#include "headers/Operator.h" // For Operator::Type
#include "headers/Scheduler.h"
#include "headers/Payload.h"
#include "headers/UpdateEvent.h"
#include "headers/util/Serializer.h"
#include "headers/util/Randomizer.h" // For randomInit tests
#include "headers/util/IdRange.h"     // For randomInit tests
#include <memory>
#include <vector>
#include <limits>
#include <cmath>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cstddef> // For std::byte
#include <nlohmann/json.hpp> // For parsing JSON to get accumulatedData

// Helper function to get accumulatedData from InOperator via toJson
// This uses a proper JSON library for robustness.
std::vector<int> getAccumulatedDataFromJson(const InOperator& op) {
    std::string jsonStr = op.toJson(false, true);
    try {
        nlohmann::json jsonObj = nlohmann::json::parse(jsonStr);
        if (jsonObj.contains("accumulatedData") && jsonObj["accumulatedData"].is_array()) {
            return jsonObj["accumulatedData"].get<std::vector<int>>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        // It's a test, so better to fail clearly if JSON is malformed.
        throw std::runtime_error(std::string("JSON parsing error in getAccumulatedDataFromJson: ") + e.what() + "\nJSON string was: " + jsonStr);
    }
    return {}; // Return empty if not found or not an array, though tests should expect it.
}

// Helper function to check if a string starts with a prefix
bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

// Helper function to check if a string ends with a suffix
bool endsWith(const std::string& text, const std::string& suffix) {
    if (text.length() < suffix.length()) return false;
    return text.compare(text.length() - suffix.length(), suffix.length(), suffix) == 0;
}

class InOperatorTest : public ::testing::Test {
protected:
    std::unique_ptr<InOperator> op;
    uint32_t operatorId = 1;
    // InOperator specific constants that might be useful in tests
    // static constexpr int MAX_CONNECTIONS = InOperator::MAX_CONNECTIONS; // If needed
    // static constexpr int MAX_DISTANCE = InOperator::MAX_DISTANCE;     // If needed

    void SetUp() override {
        op = std::make_unique<InOperator>(operatorId);
    }

    // Optional: Helper to get accumulatedData directly if a public getter existed
    // const std::vector<int>& getAccumulatedDataDirectly() const {
    //     // return op->getAccumulatedData(); // If such a method existed
    // }

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

        uint32_t getInt(uint32_t min, uint32_t max) override {
             if (uint32Index < uint32Values.size()) {
                return uint32Values[uint32Index++];
            }
            return min;
        }

        float getFloat(float min, float max) override {
            return min; // Not used by InOperator::randomInit
        }

        // Add more overrides if InOperator starts using other Randomizer methods
    };
};

// Dummy test to make sure the file compiles and links initially
TEST_F(InOperatorTest, InitialSetup) {
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->getId(), operatorId);
}

TEST_F(InOperatorTest, ConstructorAndInitialState) {
    // op is created in SetUp with operatorId
    ASSERT_NE(op, nullptr);

    // Verify ID
    EXPECT_EQ(op->getId(), operatorId);

    // Verify Operator Type
    EXPECT_EQ(op->getOpType(), Operator::Type::IN);

    // Verify accumulatedData is initially empty
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    // Verify outputConnections are initially empty
    const auto& connections = op->getOutputConnections();
    EXPECT_TRUE(connections.empty()); // Assuming DynamicArray has an empty() method or similar check
                                      // If not, might need to check size() == 0 or iterate.
                                      // Based on Operator.h, DynamicArray likely supports empty() or size().
}

TEST_F(InOperatorTest, InitialStateForDifferentId) {
    uint32_t differentId = 123;
    InOperator op_custom_id(differentId);

    EXPECT_EQ(op_custom_id.getId(), differentId);
    EXPECT_EQ(op_custom_id.getOpType(), Operator::Type::IN);
    EXPECT_TRUE(getAccumulatedDataFromJson(op_custom_id).empty());
    EXPECT_TRUE(op_custom_id.getOutputConnections().empty());
}

TEST_F(InOperatorTest, GetOpType) {
    ASSERT_NE(op, nullptr);
    EXPECT_EQ(op->getOpType(), Operator::Type::IN);

    // Test on a const version of the operator too
    const InOperator const_op(999);
    EXPECT_EQ(const_op.getOpType(), Operator::Type::IN);
}

TEST_F(InOperatorTest, MessageInt_AddPositive) {
    ASSERT_NE(op, nullptr);
    op->message(10);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 10);

    op->message(25);
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 2);
    EXPECT_EQ(accData[0], 10); // Order should be preserved
    EXPECT_EQ(accData[1], 25);
}

TEST_F(InOperatorTest, MessageInt_AddZero) {
    ASSERT_NE(op, nullptr);
    op->message(0);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 0);

    op->message(42); // Add another to ensure zero didn't break accumulation
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 2);
    EXPECT_EQ(accData[0], 0);
    EXPECT_EQ(accData[1], 42);
}

TEST_F(InOperatorTest, MessageInt_IgnoreNegative) {
    ASSERT_NE(op, nullptr);
    op->message(-10);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty()); // Should be ignored

    op->message(5); // Add a positive value
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 5);

    op->message(-7); // Add another negative
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1); // Still size 1
    EXPECT_EQ(accData[0], 5);    // Content unchanged
}

TEST_F(InOperatorTest, MessageInt_MultipleValues) {
    ASSERT_NE(op, nullptr);
    op->message(1);
    op->message(0);
    op->message(100);
    op->message(50);

    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 4);
    EXPECT_EQ(accData[0], 1);
    EXPECT_EQ(accData[1], 0);
    EXPECT_EQ(accData[2], 100);
    EXPECT_EQ(accData[3], 50);

    op->message(-5); // This should be ignored
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 4); // Size should not change
}

// --- Tests for message(float) ---

TEST_F(InOperatorTest, MessageFloat_PositiveValuesAndRounding) {
    ASSERT_NE(op, nullptr);
    op->message(10.2f); // Rounds to 10
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 10);

    op->message(20.7f); // Rounds to 21
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 2);
    EXPECT_EQ(accData[0], 10);
    EXPECT_EQ(accData[1], 21);

    op->message(30.5f); // Rounds to 31 (typically, .5 rounds up)
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 3);
    EXPECT_EQ(accData[2], 31);

    op->message(40.49f); // Rounds to 40
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 4);
    EXPECT_EQ(accData[3], 40);
}

TEST_F(InOperatorTest, MessageFloat_Zero) {
    ASSERT_NE(op, nullptr);
    op->message(0.0f);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 0);
}

TEST_F(InOperatorTest, MessageFloat_IgnoreNegative) {
    ASSERT_NE(op, nullptr);
    op->message(-10.5f);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(5.5f); // Should be 6
    op->message(-0.1f); // Ignored
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 6);
}

TEST_F(InOperatorTest, MessageFloat_OverflowAndUnderflow) {
    ASSERT_NE(op, nullptr);
    // Value that, when rounded, exceeds int max
    op->message(static_cast<float>(std::numeric_limits<int>::max()) + 1000.0f);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty()); // Should be ignored

    // Value that, when rounded, is less than int min
    op->message(static_cast<float>(std::numeric_limits<int>::min()) - 1000.0f);
    accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty()); // Should be ignored

    // Valid value to ensure list wasn't broken
    op->message(123.0f);
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 123);
}

TEST_F(InOperatorTest, MessageFloat_NaNAndInfinity) {
    ASSERT_NE(op, nullptr);
    op->message(std::numeric_limits<float>::quiet_NaN());
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty()); // Should be ignored

    op->message(std::numeric_limits<float>::infinity());
    accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty()); // Should be ignored

    op->message(-std::numeric_limits<float>::infinity());
    accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty()); // Should be ignored

    op->message(456.0f); // Valid value
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 456);
}

// --- Tests for message(double) ---

TEST_F(InOperatorTest, MessageDouble_PositiveValuesAndRounding) {
    ASSERT_NE(op, nullptr);
    op->message(10.2); // Rounds to 10
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 10);

    op->message(20.7); // Rounds to 21
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 2);
    EXPECT_EQ(accData[0], 10);
    EXPECT_EQ(accData[1], 21);

    op->message(30.5); // Rounds to 31
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 3);
    EXPECT_EQ(accData[2], 31);

    op->message(40.49); // Rounds to 40
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 4);
    EXPECT_EQ(accData[3], 40);
}

TEST_F(InOperatorTest, MessageDouble_Zero) {
    ASSERT_NE(op, nullptr);
    op->message(0.0);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 0);
}

TEST_F(InOperatorTest, MessageDouble_IgnoreNegative) {
    ASSERT_NE(op, nullptr);
    op->message(-10.5);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(5.5); // Should be 6
    op->message(-0.1); // Ignored
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 6);
}

TEST_F(InOperatorTest, MessageDouble_OverflowAndUnderflow) {
    ASSERT_NE(op, nullptr);
    op->message(static_cast<double>(std::numeric_limits<int>::max()) + 2000.0);
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(static_cast<double>(std::numeric_limits<int>::min()) - 2000.0);
    accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(789.0);
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 789);
}

TEST_F(InOperatorTest, MessageDouble_NaNAndInfinity) {
    ASSERT_NE(op, nullptr);
    op->message(std::numeric_limits<double>::quiet_NaN());
    std::vector<int> accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(std::numeric_limits<double>::infinity());
    accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(-std::numeric_limits<double>::infinity());
    accData = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accData.empty());

    op->message(101.0);
    accData = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accData.size(), 1);
    EXPECT_EQ(accData[0], 101);
}

TEST_F(InOperatorTest, ProcessData_ClearsAccumulatedData) {
    ASSERT_NE(op, nullptr);
    op->message(10);
    op->message(20);

    std::vector<int> accDataBefore = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accDataBefore.size(), 2);

    // To test payload generation, we'd ideally mock Scheduler.
    // For now, we focus on InOperator's state change: accumulatedData clearing.
    // We need to add at least one connection for processData to attempt scheduling.
    // Operator::addConnectionInternal is protected.
    // We can use randomInit with a mock randomizer to add a controlled connection.
    // Or, we can assume that if outputConnections is not empty, it tries to schedule.

    // Let's add a connection using randomInit (which calls addConnectionInternal).
    // We need a predictable way to add a connection.
    // Since randomInit(uint32_t, Randomizer*) is empty, we use the IdRange version.
    // We'll create a dummy IdRange and a MockRandomizer that ensures one connection.
    IdRange testRange(100, 200); // Dummy range
    MockRandomizer mockRng({1, 5}, {101}); // 1 connection, distance 5, targetId 101
                                         // First int for connectionsToAttempt, second for distance.
                                         // First uint32_t for targetId.
    op->randomInit(&testRange, &mockRng);

    ASSERT_FALSE(op->getOutputConnections().empty()) << "Test setup failed: No connections added for processData test.";

    op->processData();

    std::vector<int> accDataAfter = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accDataAfter.empty()) << "accumulatedData should be empty after processData.";
}

TEST_F(InOperatorTest, ProcessData_WhenAccumulatedDataIsEmpty) {
    ASSERT_NE(op, nullptr);
    // Ensure accumulatedData is empty initially
    std::vector<int> accDataBefore = getAccumulatedDataFromJson(*op);
    ASSERT_TRUE(accDataBefore.empty());

    // Add a connection so it would attempt to process if data existed
    IdRange testRange(100, 200);
    MockRandomizer mockRng({1, 3}, {102});
    op->randomInit(&testRange, &mockRng);
    ASSERT_FALSE(op->getOutputConnections().empty());

    op->processData(); // Should run without error

    std::vector<int> accDataAfter = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accDataAfter.empty()) << "accumulatedData should remain empty.";
}

TEST_F(InOperatorTest, ProcessData_NoOutputConnections) {
    ASSERT_NE(op, nullptr);
    op->message(50);
    op->message(60);

    // Ensure no connections exist
    ASSERT_TRUE(op->getOutputConnections().empty()) << "Test setup assumption failed: Operator should have no connections for this test.";

    std::vector<int> accDataBefore = getAccumulatedDataFromJson(*op);
    ASSERT_EQ(accDataBefore.size(), 2);

    op->processData(); // Should run without error and clear data

    std::vector<int> accDataAfter = getAccumulatedDataFromJson(*op);
    EXPECT_TRUE(accDataAfter.empty()) << "accumulatedData should be cleared even with no output connections.";
}

TEST_F(InOperatorTest, ProcessData_MultipleItemsProcessed) {
    // This test primarily verifies that accumulatedData is cleared,
    // implying all items were iterated over.
    ASSERT_NE(op, nullptr);
    op->message(1);
    op->message(2);
    op->message(3);

    IdRange testRange(200, 300);
    MockRandomizer mockRng({1, 1}, {201}); // Add one connection
    op->randomInit(&testRange, &mockRng);
    ASSERT_FALSE(op->getOutputConnections().empty());

    op->processData();
    EXPECT_TRUE(getAccumulatedDataFromJson(*op).empty());
}

// --- Tests for toJson ---

TEST_F(InOperatorTest, ToJson_NoPretty_Enclosed_NoData) {
    ASSERT_NE(op, nullptr);
    // No data added to accumulatedData, should be empty array

    std::string jsonOutput = op->toJson(false, true);
    EXPECT_TRUE(startsWith(jsonOutput, "{"));
    EXPECT_TRUE(endsWith(jsonOutput, "}"));
    EXPECT_EQ(jsonOutput.find("\n"), std::string::npos); // No newlines

    // Validate JSON structure and content
    try {
        nlohmann::json j = nlohmann::json::parse(jsonOutput);
        EXPECT_EQ(j["opType"], "IN");
        EXPECT_EQ(j["operatorId"], operatorId);
        ASSERT_TRUE(j["outputDistanceBuckets"].is_array());
        EXPECT_TRUE(j["outputDistanceBuckets"].empty()); // Assuming no connections added in SetUp
        ASSERT_TRUE(j["accumulatedData"].is_array());
        EXPECT_TRUE(j["accumulatedData"].empty());
    } catch (const nlohmann::json::parse_error& e) {
        FAIL() << "JSON parsing failed: " << e.what() << "\nJSON string: " << jsonOutput;
    }
}

TEST_F(InOperatorTest, ToJson_NoPretty_Enclosed_WithData) {
    ASSERT_NE(op, nullptr);
    op->message(10);
    op->message(20);

    std::string jsonOutput = op->toJson(false, true);
    EXPECT_TRUE(startsWith(jsonOutput, "{"));
    EXPECT_TRUE(endsWith(jsonOutput, "}"));
    EXPECT_EQ(jsonOutput.find("\n"), std::string::npos);

    try {
        nlohmann::json j = nlohmann::json::parse(jsonOutput);
        EXPECT_EQ(j["opType"], "IN");
        EXPECT_EQ(j["operatorId"], operatorId);
        EXPECT_TRUE(j["outputDistanceBuckets"].is_array()); // Assuming still empty
        ASSERT_TRUE(j["accumulatedData"].is_array());
        ASSERT_EQ(j["accumulatedData"].size(), 2);
        EXPECT_EQ(j["accumulatedData"][0], 10);
        EXPECT_EQ(j["accumulatedData"][1], 20);
    } catch (const nlohmann::json::parse_error& e) {
        FAIL() << "JSON parsing failed: " << e.what() << "\nJSON string: " << jsonOutput;
    }
}

TEST_F(InOperatorTest, ToJson_Pretty_Enclosed_WithData) {
    ASSERT_NE(op, nullptr);
    op->message(30);
    op->message(40);

    std::string jsonOutput = op->toJson(true, true); // prettyPrint = true
    EXPECT_TRUE(startsWith(jsonOutput, "{"));
    EXPECT_TRUE(endsWith(jsonOutput, "\n}")); // Pretty print adds newline before closing bracket
    EXPECT_NE(jsonOutput.find("\n"), std::string::npos); // Should contain newlines
    EXPECT_NE(jsonOutput.find("  "), std::string::npos);  // Should contain indentation

    try {
        nlohmann::json j = nlohmann::json::parse(jsonOutput);
        EXPECT_EQ(j["opType"], "IN");
        EXPECT_EQ(j["operatorId"], operatorId);
        ASSERT_TRUE(j["accumulatedData"].is_array());
        ASSERT_EQ(j["accumulatedData"].size(), 2);
        EXPECT_EQ(j["accumulatedData"][0], 30);
        EXPECT_EQ(j["accumulatedData"][1], 40);
    } catch (const nlohmann::json::parse_error& e) {
        FAIL() << "JSON parsing failed: " << e.what() << "\nJSON string: " << jsonOutput;
    }
}

TEST_F(InOperatorTest, ToJson_NoPretty_NotEnclosed_WithData) {
    ASSERT_NE(op, nullptr);
    op->message(50);

    std::string jsonOutput = op->toJson(false, false); // encloseInBrackets = false
    EXPECT_FALSE(startsWith(jsonOutput, "{"));
    EXPECT_FALSE(endsWith(jsonOutput, "}"));
    EXPECT_EQ(jsonOutput.find("\n"), std::string::npos);

    // When not enclosed, the output is a fragment. We need to wrap it to parse.
    std::string wrappedJson = "{" + jsonOutput + "}";
    try {
        nlohmann::json j = nlohmann::json::parse(wrappedJson);
        EXPECT_EQ(j["opType"], "IN");
        EXPECT_EQ(j["operatorId"], operatorId);
        ASSERT_TRUE(j["accumulatedData"].is_array());
        ASSERT_EQ(j["accumulatedData"].size(), 1);
        EXPECT_EQ(j["accumulatedData"][0], 50);
    } catch (const nlohmann::json::parse_error& e) {
        FAIL() << "JSON parsing failed for wrapped fragment: " << e.what() << "\nOriginal fragment: " << jsonOutput;
    }
}

// --- Tests for equals ---

TEST_F(InOperatorTest, Equals_IdenticalFreshOperators) {
    ASSERT_NE(op, nullptr); // op has id = operatorId (1)
    InOperator op2(operatorId); // Same ID

    EXPECT_TRUE(op->equals(op2)) << "Freshly created InOperators with same ID should be equal.";
    EXPECT_TRUE(op2.equals(*op));
    EXPECT_TRUE(*op == op2) << "Non-member operator== should also report true.";
}

TEST_F(InOperatorTest, Equals_DifferentIds) {
    ASSERT_NE(op, nullptr); // op has id = operatorId (1)
    InOperator op_diff_id(operatorId + 1); // Different ID

    EXPECT_FALSE(op->equals(op_diff_id)) << "InOperators with different IDs should not be equal.";
    EXPECT_FALSE(*op == op_diff_id);
}

TEST_F(InOperatorTest, Equals_AccumulatedDataDoesNotAffectEquality) {
    ASSERT_NE(op, nullptr);
    InOperator op2(operatorId);

    op->message(10); // op now has accumulatedData
    // op2 has no accumulatedData

    // As per InOperator.cpp, accumulatedData is transient and not part of equals.
    EXPECT_TRUE(op->equals(op2)) << "Equality should hold true even if accumulatedData differs.";
    EXPECT_TRUE(op2.equals(*op));
    EXPECT_TRUE(*op == op2);

    op2.message(20); // op2 now has different accumulatedData
    EXPECT_TRUE(op->equals(op2)) << "Equality should still hold true with different non-empty accumulatedData.";
    EXPECT_TRUE(*op == op2);
}

TEST_F(InOperatorTest, Equals_DifferentOperatorTypes) {
    ASSERT_NE(op, nullptr); // InOperator
    AddOperator add_op(operatorId); // AddOperator with same ID

    // op->equals(add_op) would be a compile error or bad cast if Operator::equals isn't careful.
    // The primary way to check polymorphic equality is non-member operator==
    EXPECT_FALSE(*op == add_op) << "InOperator should not be equal to AddOperator, even with same ID.";
}

TEST_F(InOperatorTest, Equals_WithIdenticalConnections) {
    InOperator op1_conn(200);
    InOperator op2_conn(200); // Same ID

    IdRange range(10, 20);
    // Add connection (target 15, distance 3) to op1_conn
    MockRandomizer rng1({1, 3}, {15}); // attempts=1, distance=3, targetId=15
    op1_conn.randomInit(&range, &rng1);

    // Add the exact same connection to op2_conn
    MockRandomizer rng2({1, 3}, {15}); // attempts=1, distance=3, targetId=15
    op2_conn.randomInit(&range, &rng2);

    ASSERT_FALSE(op1_conn.getOutputConnections().empty());
    ASSERT_FALSE(op2_conn.getOutputConnections().empty());

    EXPECT_TRUE(op1_conn.equals(op2_conn)) << "Operators with same ID and identical connections should be equal.";
    EXPECT_TRUE(op1_conn == op2_conn);
}

TEST_F(InOperatorTest, Equals_WithDifferentConnections) {
    InOperator op1_conn_diff(201);
    InOperator op2_conn_diff(201); // Same ID

    IdRange range(30, 40);
    // Add connection (target 35, distance 2) to op1_conn_diff
    MockRandomizer rng1({1, 2}, {35});
    op1_conn_diff.randomInit(&range, &rng1);

    // Add a different connection (target 36, distance 4) to op2_conn_diff
    MockRandomizer rng2({1, 4}, {36});
    op2_conn_diff.randomInit(&range, &rng2);

    ASSERT_FALSE(op1_conn_diff.getOutputConnections().empty());
    ASSERT_FALSE(op2_conn_diff.getOutputConnections().empty());

    EXPECT_FALSE(op1_conn_diff.equals(op2_conn_diff)) << "Operators with same ID but different connections should not be equal.";
    EXPECT_FALSE(op1_conn_diff == op2_conn_diff);
}

TEST_F(InOperatorTest, Equals_OneWithConnectionsOneWithout) {
    InOperator op_no_conn(202);
    InOperator op_with_conn(202); // Same ID

    IdRange range(50, 60);
    // Add connection to op_with_conn
    MockRandomizer rng({1, 1}, {55});
    op_with_conn.randomInit(&range, &rng);

    ASSERT_TRUE(op_no_conn.getOutputConnections().empty());
    ASSERT_FALSE(op_with_conn.getOutputConnections().empty());

    EXPECT_FALSE(op_no_conn.equals(op_with_conn)) << "Operator with no connections should not equal one with connections (same ID).";
    EXPECT_FALSE(op_no_conn == op_with_conn);
    EXPECT_FALSE(op_with_conn.equals(op_no_conn)); // Check reverse
    EXPECT_FALSE(op_with_conn == op_no_conn);
}

// --- Tests for Serialization/Deserialization ---

TEST_F(InOperatorTest, SerializeDeserialize_BasicNoConnections) {
    std::unique_ptr<InOperator> op_orig = std::make_unique<InOperator>(101);
    op_orig->message(50); // This should not be part of serialized state

    // Serialize
    std::vector<std::byte> serialized_data = op_orig->serializeToBytes();
    ASSERT_GE(serialized_data.size(), static_cast<size_t>(4 + 2 + 4)); // Size_prefix + Type + ID

    const std::byte* current_ptr = serialized_data.data();
    const std::byte* stream_end = current_ptr + serialized_data.size();

    // Simulate MetaController reading total size (advances current_ptr by 4)
    uint32_t total_data_size = Serializer::read_uint32(current_ptr, stream_end);
    ASSERT_EQ(total_data_size, serialized_data.size() - 4);

    // Simulate MetaController reading OperatorType (advances current_ptr by 2)
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current_ptr, stream_end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::IN);

    // Now current_ptr points to the start of Operator ID, as expected by Operator's deserialization constructor.
    // The 'end' for the constructor should be the end of this specific operator's data block.
    const std::byte* operator_data_block_end = serialized_data.data() + 4 + total_data_size;

    std::unique_ptr<InOperator> op_deserialized;
    ASSERT_NO_THROW({
        op_deserialized = std::make_unique<InOperator>(current_ptr, operator_data_block_end);
    });

    ASSERT_NE(op_deserialized, nullptr);

    // Check equality (should be true as accumulatedData is ignored, and no connections)
    EXPECT_TRUE(op_orig->equals(*op_deserialized));
    EXPECT_TRUE(*op_orig == *op_deserialized);

    // Verify ID
    EXPECT_EQ(op_deserialized->getId(), op_orig->getId());
    // Verify opType
    EXPECT_EQ(op_deserialized->getOpType(), Operator::Type::IN);
    // Verify accumulatedData is empty in deserialized object
    EXPECT_TRUE(getAccumulatedDataFromJson(*op_deserialized).empty());
    // Verify connections are empty
    EXPECT_TRUE(op_deserialized->getOutputConnections().empty());
}

TEST_F(InOperatorTest, SerializeDeserialize_WithConnections) {
    std::unique_ptr<InOperator> op_orig = std::make_unique<InOperator>(102);
    op_orig->message(75); // Should not be serialized

    // Add connections to op_orig
    IdRange range(500, 600);
    // MockRandomizer: intValues for {attempts, dist1, dist2, ...}, uint32Values for {id1, id2, ...}
    // randomInit calls: getInt(0,MAX_CONN) for attempts, then for each attempt:
    //                   getInt(minId, maxId) for targetId (uint32_t)
    //                   getInt(0, MAX_DIST) for distance (int)
    // So for 2 connections: {attempts=2, dist_conn1, dist_conn2}, {targetId_conn1, targetId_conn2}
    MockRandomizer rng({2, 1, 2}, {505, 510});
    op_orig->randomInit(&range, &rng);
    ASSERT_FALSE(op_orig->getOutputConnections().empty());
    ASSERT_EQ(op_orig->getOutputConnections().size(), 2);


    // Serialize
    std::vector<std::byte> serialized_data = op_orig->serializeToBytes();
    ASSERT_GE(serialized_data.size(), static_cast<size_t>(4 + 2 + 4)); // Min size

    const std::byte* current_ptr = serialized_data.data();
    const std::byte* stream_end = current_ptr + serialized_data.size();

    uint32_t total_data_size = Serializer::read_uint32(current_ptr, stream_end);
    ASSERT_EQ(total_data_size, serialized_data.size() - 4);
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current_ptr, stream_end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::IN);

    const std::byte* operator_data_block_end = serialized_data.data() + 4 + total_data_size;

    std::unique_ptr<InOperator> op_deserialized;
    ASSERT_NO_THROW({
        op_deserialized = std::make_unique<InOperator>(current_ptr, operator_data_block_end);
    });

    ASSERT_NE(op_deserialized, nullptr);

    // Check equality (should be true as accumulatedData ignored, connections should match)
    EXPECT_TRUE(op_orig->equals(*op_deserialized));
    EXPECT_TRUE(*op_orig == *op_deserialized);

    // Verify ID
    EXPECT_EQ(op_deserialized->getId(), op_orig->getId());
    // Verify accumulatedData is empty
    EXPECT_TRUE(getAccumulatedDataFromJson(*op_deserialized).empty());
    // Verify connections are preserved (deep comparison is handled by Operator::equals)
    EXPECT_EQ(op_deserialized->getOutputConnections().size(), op_orig->getOutputConnections().size());
}

TEST_F(InOperatorTest, SerializeDeserialize_MaxId) {
    std::unique_ptr<InOperator> op_orig = std::make_unique<InOperator>(std::numeric_limits<uint32_t>::max());
    op_orig->message(10); // Should not be serialized

    std::vector<std::byte> serialized_data = op_orig->serializeToBytes();

    const std::byte* current_ptr = serialized_data.data();
    const std::byte* stream_end = current_ptr + serialized_data.size();

    uint32_t total_data_size = Serializer::read_uint32(current_ptr, stream_end);
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current_ptr, stream_end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::IN);

    const std::byte* operator_data_block_end = serialized_data.data() + 4 + total_data_size;
    std::unique_ptr<InOperator> op_deserialized;
    ASSERT_NO_THROW({
        op_deserialized = std::make_unique<InOperator>(current_ptr, operator_data_block_end);
    });

    ASSERT_NE(op_deserialized, nullptr);
    EXPECT_TRUE(op_orig->equals(*op_deserialized));
    EXPECT_EQ(op_deserialized->getId(), std::numeric_limits<uint32_t>::max());
    EXPECT_TRUE(getAccumulatedDataFromJson(*op_deserialized).empty());
}

// --- Tests for randomInit ---

TEST_F(InOperatorTest, RandomInit_IdRange_AddsConnections) {
    ASSERT_NE(op, nullptr);
    IdRange range(100, 200); // Target IDs will be within this range

    // MockRandomizer setup:
    // intValues for: {connectionsToAttempt, distance1, distance2, ...}
    // uint32Values for: {targetId1, targetId2, ...}
    MockRandomizer mockRng({2, 5, 8}, {101, 150});
    // Attempt 2 connections:
    // 1. Target 101, distance 5
    // 2. Target 150, distance 8

    op->randomInit(&range, &mockRng);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.size(), 2) << "Should have added 2 connections based on mock RNG.";

    const auto* bucket_dist5 = connections.get(5); // Assuming get(distance) returns the set ptr
    ASSERT_NE(bucket_dist5, nullptr) << "Distance bucket 5 should exist.";
    EXPECT_NE(bucket_dist5->find(101), bucket_dist5->end()) << "Target ID 101 should be in distance 5 bucket.";

    const auto* bucket_dist8 = connections.get(8);
    ASSERT_NE(bucket_dist8, nullptr) << "Distance bucket 8 should exist.";
    EXPECT_NE(bucket_dist8->find(150), bucket_dist8->end()) << "Target ID 150 should be in distance 8 bucket.";
}

TEST_F(InOperatorTest, RandomInit_IdRange_ZeroConnectionsAttempted) {
    ASSERT_NE(op, nullptr);
    IdRange range(100, 200);
    MockRandomizer mockRng({0}, {}); // Attempt 0 connections

    op->randomInit(&range, &mockRng);
    EXPECT_TRUE(op->getOutputConnections().empty()) << "Should have no connections if RNG attempts 0.";
}

TEST_F(InOperatorTest, RandomInit_IdRange_MaxConnectionsIsZeroBehavior) {
    // This test assumes InOperator::MAX_CONNECTIONS could be 0.
    // If InOperator::MAX_CONNECTIONS is a compile-time constant > 0,
    // then the behavior tested is when rng->getInt(0, MAX_CONNECTIONS) returns 0.
    // This is covered by ZeroConnectionsAttempted.
    // If MAX_CONNECTIONS itself was 0, randomInit should return early.
    // The current InOperator.cpp:
    // `if (MAX_CONNECTIONS <= 0) { return; }`
    // `int connectionsToAttempt = rng->getInt(0, MAX_CONNECTIONS);`
    // `if (connectionsToAttempt == 0) { return; }`
    // So, if MAX_CONNECTIONS is 0 (or less), it returns immediately.
    // This test relies on MAX_CONNECTIONS being > 0 and rng returning 0 attempts,
    // which is already tested by RandomInit_IdRange_ZeroConnectionsAttempted.

    ASSERT_NE(op, nullptr);
    IdRange range(100, 200);
    // If MAX_CONNECTIONS was 0, rng->getInt(0,0) would yield 0.
    MockRandomizer mockRng({0}, {}); // Simulates 0 connections chosen
    op->randomInit(&range, &mockRng);
    EXPECT_TRUE(op->getOutputConnections().empty());
}

TEST_F(InOperatorTest, RandomInit_MaxOperatorId_IsEmptyImplementation) {
    ASSERT_NE(op, nullptr);
    // This version of randomInit is empty in InOperator.cpp
    // So, calling it should not change any state (e.g., connections).
    MockRandomizer emptyMockRng({}, {}); // No specific random numbers needed

    op->randomInit(1000, &emptyMockRng); // Pass some maxOperatorId

    EXPECT_TRUE(op->getOutputConnections().empty()) << "The empty randomInit(uint32_t, Randomizer*) should not add connections.";

    // Add a connection via the other randomInit to ensure the operator is not broken
    IdRange range(10,20);
    MockRandomizer mockRng_add({1,1},{15});
    op->randomInit(&range, &mockRng_add);
    ASSERT_FALSE(op->getOutputConnections().empty()) << "Sanity check: Adding connection via other randomInit should still work.";

    // Call the empty one again
    op->randomInit(2000, &emptyMockRng);
    ASSERT_FALSE(op->getOutputConnections().empty()) << "Calling empty randomInit should not remove existing connections.";
    EXPECT_EQ(op->getOutputConnections().size(), 1) << "Size of connections should remain unchanged.";
}

TEST_F(InOperatorTest, ToJson_Pretty_NotEnclosed_WithData) {
    ASSERT_NE(op, nullptr);
    op->message(60);
    op->message(70);

    std::string jsonOutput = op->toJson(true, false); // prettyPrint = true, encloseInBrackets = false
    EXPECT_FALSE(startsWith(jsonOutput, "{"));
    // Ends with the last part of accumulatedData array, possibly with a newline before it from base toJson, but not "} "
    EXPECT_FALSE(endsWith(jsonOutput, "}"));
    EXPECT_FALSE(endsWith(jsonOutput, "\n}"));
    EXPECT_NE(jsonOutput.find("\n"), std::string::npos);

    std::string wrappedJson = "{" + jsonOutput + "}";
    try {
        nlohmann::json j = nlohmann::json::parse(wrappedJson);
        EXPECT_EQ(j["opType"], "IN");
        EXPECT_EQ(j["operatorId"], operatorId);
        ASSERT_TRUE(j["accumulatedData"].is_array());
        ASSERT_EQ(j["accumulatedData"].size(), 2);
        EXPECT_EQ(j["accumulatedData"][0], 60);
        EXPECT_EQ(j["accumulatedData"][1], 70);
    } catch (const nlohmann::json::parse_error& e) {
        FAIL() << "JSON parsing failed for wrapped fragment: " << e.what() << "\nOriginal fragment: " << jsonOutput;
    }
}
