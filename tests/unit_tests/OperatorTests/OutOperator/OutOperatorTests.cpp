#include <gtest/gtest.h>
#include "operators/OutOperator.h" // Adjust path as necessary based on include paths in CMake
#include "operators/AddOperator.h"   // For cross-type equality test
#include "helpers/JsonTestHelpers.h" // For JSON comparison
#include "helpers/MockRandomizer.h"  // For MockRandomizer
#include "util/IdRange.h"          // For IdRange
#include "util/Serializer.h"       // For serialization tests
#include <limits> // Required for std::numeric_limits
#include <cmath>  // Required for std::round, std::isnan, std::isinf (though indirectly via OutOperator)
#include <string> // Required for std::string
#include <vector> // Required for std::vector
#include <cstddef> // Required for std::byte
// #include "nlohmann/json.hpp" // Removed this include

// Anonymous namespace for helper functions specific to this test file
namespace {
    std::string MOCK_FILE_DIR = "../tests/unit_tests/golden_files/outOperator/";

    char expectedCharFromInt(int value) {
        constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
        constexpr int CHAR_BITS = 8;
        constexpr int SHIFT_AMOUNT = (INT_VALUE_BITS > CHAR_BITS) ? (INT_VALUE_BITS - CHAR_BITS) : 0;
        int processed_value = (value < 0) ? 0 : value;
        if constexpr (SHIFT_AMOUNT > 0) {
            return static_cast<char>(static_cast<unsigned int>(processed_value) >> SHIFT_AMOUNT);
        } else {
            return static_cast<char>(processed_value & 0xFF);
        }
    }

    int inputValueForChar(unsigned char target_char_val) {
        constexpr int INT_VALUE_BITS = std::numeric_limits<int>::digits;
        constexpr int CHAR_BITS = 8;
        constexpr int SHIFT_AMOUNT = (INT_VALUE_BITS > CHAR_BITS) ? (INT_VALUE_BITS - CHAR_BITS) : 0;
        if (target_char_val == 0) {
            if (SHIFT_AMOUNT > 0) return 0;
            return 0;
        }
        if constexpr (SHIFT_AMOUNT > 0) {
            return static_cast<int>(static_cast<unsigned int>(target_char_val) << SHIFT_AMOUNT);
        } else {
            return static_cast<int>(target_char_val);
        }
    }
} // end anonymous namespace


class OutOperatorTest : public ::testing::Test {
protected:
    OutOperator op{1};

    // Updated to use JsonTestHelpers and return std::vector<int>
    std::vector<int> getOperatorDataJson(OutOperator& operator_instance) {
        // toJson(prettyPrint=false, encloseInBrackets=true) is the default for toJson()
        std::string json_str = operator_instance.toJson(false, true);
        return JsonTestHelpers::getJsonArrayIntValue(json_str, "data");
    }
};

TEST_F(OutOperatorTest, GetOpTypeReturnsCorrectType) {
    // op is created by the OutOperatorTest fixture
    ASSERT_EQ(op.getOpType(), Operator::Type::OUT);

    // Also test with a locally created instance for good measure
    OutOperator local_op(999);
    ASSERT_EQ(local_op.getOpType(), Operator::Type::OUT);
}

// Test suite for message handling specifically
class OutOperatorMessageTests : public OutOperatorTest {
};

TEST_F(OutOperatorMessageTests, HandlesIntMessages) {
    OutOperator local_op(1);
    local_op.message(10);
    local_op.message(-5);
    local_op.message(0);

    std::vector<int> data_array = getOperatorDataJson(local_op); // Now returns std::vector<int>
    ASSERT_EQ(data_array.size(), 3);
    ASSERT_EQ(data_array[0], 10);    // Direct comparison
    ASSERT_EQ(data_array[1], -5);   // Direct comparison
    ASSERT_EQ(data_array[2], 0);    // Direct comparison

    OutOperator op_max_min(2);
    op_max_min.message(std::numeric_limits<int>::max());
    op_max_min.message(std::numeric_limits<int>::min());

    data_array = getOperatorDataJson(op_max_min); // Now returns std::vector<int>
    ASSERT_EQ(data_array.size(), 2);
    ASSERT_EQ(data_array[0], std::numeric_limits<int>::max()); // Direct comparison
    ASSERT_EQ(data_array[1], std::numeric_limits<int>::min()); // Direct comparison
}

TEST_F(OutOperatorMessageTests, HandlesFloatMessagesRounding) {
    OutOperator local_op(1);
    local_op.message(10.2f);
    local_op.message(10.7f);
    local_op.message(-5.3f);
    local_op.message(-5.8f);
    local_op.message(0.0f);
    local_op.message(0.4f);
    local_op.message(0.6f);

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 7);
    ASSERT_EQ(data_array[0], 10);
    ASSERT_EQ(data_array[1], 11);
    ASSERT_EQ(data_array[2], -5);
    ASSERT_EQ(data_array[3], -6);
    ASSERT_EQ(data_array[4], 0);
    ASSERT_EQ(data_array[5], 0);
    ASSERT_EQ(data_array[6], 1);
}

TEST_F(OutOperatorMessageTests, HandlesFloatMessagesClamping) {
    OutOperator local_op(1);
    local_op.message(std::numeric_limits<float>::max());
    local_op.message(std::numeric_limits<float>::lowest());
    local_op.message(static_cast<float>(std::numeric_limits<int>::max()) + 100.0f);
    local_op.message(static_cast<float>(std::numeric_limits<int>::min()) - 100.0f);

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 4);
    ASSERT_EQ(data_array[0], std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[1], std::numeric_limits<int>::min());
    ASSERT_EQ(data_array[2], std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[3], std::numeric_limits<int>::min());
}

TEST_F(OutOperatorMessageTests, HandlesFloatMessagesSpecialValues) {
    OutOperator local_op(1);
    local_op.message(1);
    local_op.message(std::numeric_limits<float>::quiet_NaN());
    local_op.message(std::numeric_limits<float>::infinity());
    local_op.message(-std::numeric_limits<float>::infinity());

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 1);
    ASSERT_EQ(data_array[0], 1);
}

TEST_F(OutOperatorMessageTests, HandlesDoubleMessagesRounding) {
    OutOperator local_op(1);
    local_op.message(20.2);
    local_op.message(20.7);
    local_op.message(-15.3);
    local_op.message(-15.8);
    local_op.message(0.0);
    local_op.message(0.4);
    local_op.message(0.6);

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 7);
    ASSERT_EQ(data_array[0], 20);
    ASSERT_EQ(data_array[1], 21);
    ASSERT_EQ(data_array[2], -15);
    ASSERT_EQ(data_array[3], -16);
    ASSERT_EQ(data_array[4], 0);
    ASSERT_EQ(data_array[5], 0);
    ASSERT_EQ(data_array[6], 1);
}

TEST_F(OutOperatorMessageTests, HandlesDoubleMessagesClamping) {
    OutOperator local_op(1);
    local_op.message(std::numeric_limits<double>::max());
    local_op.message(std::numeric_limits<double>::lowest());
    local_op.message(static_cast<double>(std::numeric_limits<int>::max()) + 100.0);
    local_op.message(static_cast<double>(std::numeric_limits<int>::min()) - 100.0);

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 4);
    ASSERT_EQ(data_array[0], std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[1], std::numeric_limits<int>::min());
    ASSERT_EQ(data_array[2], std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[3], std::numeric_limits<int>::min());
}

TEST_F(OutOperatorMessageTests, HandlesDoubleMessagesSpecialValues) {
    OutOperator local_op(1);
    local_op.message(2);
    local_op.message(std::numeric_limits<double>::quiet_NaN());
    local_op.message(std::numeric_limits<double>::infinity());
    local_op.message(-std::numeric_limits<double>::infinity());

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 1);
    ASSERT_EQ(data_array[0], 2);
}

TEST_F(OutOperatorMessageTests, HandlesMixedTypeMessages) {
    OutOperator local_op(1);
    local_op.message(100);
    local_op.message(20.7f);
    local_op.message(-30.3);
    local_op.message(std::numeric_limits<int>::max());
    local_op.message(0.4f);
    local_op.message(static_cast<double>(std::numeric_limits<int>::min()) - 2000.0);

    std::vector<int> data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 6);
    ASSERT_EQ(data_array[0], 100);
    ASSERT_EQ(data_array[1], 21);
    ASSERT_EQ(data_array[2], -30);
    ASSERT_EQ(data_array[3], std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[4], 0);
    ASSERT_EQ(data_array[5], std::numeric_limits<int>::min());
}

TEST_F(OutOperatorTest, EmptyOperatorToJsonCheck) {
    OutOperator local_op(1);
    std::string json_str = local_op.toJson();

    ASSERT_EQ(JsonTestHelpers::getJsonIntValue(json_str, "operatorId"), 1);
    ASSERT_NE(json_str.find("\"opType\":\"" + Operator::typeToString(Operator::Type::OUT) + "\""), std::string::npos);
    ASSERT_NE(json_str.find("\"outputDistanceBuckets\":[]"), std::string::npos);

    std::vector<int> data_vec = JsonTestHelpers::getJsonArrayIntValue(json_str, "data");
    ASSERT_TRUE(data_vec.empty());
}

TEST_F(OutOperatorTest, HasOutputFalseWhenEmpty) {
    OutOperator local_op(100);
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorTest, HasOutputTrueWhenNotEmptyAfterIntMessage) {
    OutOperator local_op(101);
    local_op.message(123);
    ASSERT_TRUE(local_op.hasOutput());
}

TEST_F(OutOperatorTest, HasOutputTrueWhenNotEmptyAfterFloatMessage) {
    OutOperator local_op(102);
    local_op.message(123.45f);
    ASSERT_TRUE(local_op.hasOutput());
}

TEST_F(OutOperatorTest, HasOutputTrueWhenNotEmptyAfterDoubleMessage) {
    OutOperator local_op(103);
    local_op.message(678.90);
    ASSERT_TRUE(local_op.hasOutput());
}

TEST_F(OutOperatorTest, HasOutputFalseIfMessageIsNaN) {
    OutOperator local_op(104);
    local_op.message(std::numeric_limits<float>::quiet_NaN());
    ASSERT_FALSE(local_op.hasOutput());

    local_op.message(123);
    ASSERT_TRUE(local_op.hasOutput());
    local_op.message(std::numeric_limits<double>::infinity());
    ASSERT_TRUE(local_op.hasOutput());
}

TEST_F(OutOperatorTest, HasOutputFalseAfterGetDataAsStringClearsBufferCorrected) {
    OutOperator local_op(105);
    local_op.message(100);
    ASSERT_TRUE(local_op.hasOutput());
    std::string data_str = local_op.getDataAsString();
    ASSERT_FALSE(local_op.hasOutput());

    std::string data_str_after_clear = local_op.getDataAsString();
    ASSERT_TRUE(data_str_after_clear.empty());
}

TEST_F(OutOperatorTest, HasOutputRemainsFalseIfGetDataAsStringCalledOnEmpty) {
    OutOperator local_op(106);
    ASSERT_FALSE(local_op.hasOutput());
    local_op.getDataAsString();
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorTest, ProcessDataDoesNotAlterDataBuffer) {
    OutOperator local_op(200);
    local_op.message(10);
    local_op.message(20.5f);

    std::vector<int> data_before = getOperatorDataJson(local_op); // Returns std::vector<int>
    bool has_output_before = local_op.hasOutput();

    local_op.processData();

    std::vector<int> data_after = getOperatorDataJson(local_op); // Returns std::vector<int>
    bool has_output_after = local_op.hasOutput();

    ASSERT_EQ(data_before, data_after); // Compares std::vector<int>
    ASSERT_EQ(has_output_before, has_output_after);
    ASSERT_TRUE(has_output_after);
    ASSERT_EQ(data_after.size(), 2);
    ASSERT_EQ(data_after[0], 10);
    ASSERT_EQ(data_after[1], 21);
}

TEST_F(OutOperatorTest, ProcessDataOnEmptyBufferHasNoEffect) {
    OutOperator local_op(201);
    ASSERT_FALSE(local_op.hasOutput());

    std::vector<int> data_before = getOperatorDataJson(local_op); // Returns std::vector<int>

    local_op.processData();

    std::vector<int> data_after = getOperatorDataJson(local_op); // Returns std::vector<int>
    bool has_output_after = local_op.hasOutput();

    ASSERT_EQ(data_before, data_after);
    ASSERT_TRUE(data_before.empty());
    ASSERT_TRUE(data_after.empty());
    ASSERT_FALSE(has_output_after);
}

class OutOperatorGetDataAsStringTests : public OutOperatorTest {};

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringEmptyBuffer) {
    OutOperator local_op(300);
    std::string result = local_op.getDataAsString();
    ASSERT_TRUE(result.empty());
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringClearsBuffer) {
    OutOperator local_op(301);
    local_op.message(inputValueForChar('X'));

    std::string result1 = local_op.getDataAsString();
    ASSERT_EQ(result1.length(), 1);
    ASSERT_EQ(result1[0], 'X');
    ASSERT_FALSE(local_op.hasOutput());

    std::string result2 = local_op.getDataAsString();
    ASSERT_TRUE(result2.empty());
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringZeroValue) {
    OutOperator local_op(302);
    local_op.message(0);
    std::string result = local_op.getDataAsString();
    ASSERT_EQ(result.length(), 1);
    ASSERT_EQ(static_cast<unsigned char>(result[0]), 0);
    ASSERT_EQ(result[0], expectedCharFromInt(0));
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringMaxIntValue) {
    OutOperator local_op(303);
    local_op.message(std::numeric_limits<int>::max());
    std::string result = local_op.getDataAsString();
    ASSERT_EQ(result.length(), 1);
    ASSERT_EQ(static_cast<unsigned char>(result[0]), 255);
    ASSERT_EQ(result[0], expectedCharFromInt(std::numeric_limits<int>::max()));
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringSpecificCharA) {
    OutOperator local_op(304);
    int inputValue = inputValueForChar('A');
    local_op.message(inputValue);
    std::string result = local_op.getDataAsString();
    ASSERT_EQ(result.length(), 1);
    ASSERT_EQ(result[0], 'A');
    ASSERT_EQ(result[0], expectedCharFromInt(inputValue));
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringSpecificChar128) {
    OutOperator local_op(305);
    unsigned char target_val = 128;
    int inputValue = inputValueForChar(target_val);
    local_op.message(inputValue);
    std::string result = local_op.getDataAsString();
    ASSERT_EQ(result.length(), 1);
    ASSERT_EQ(static_cast<unsigned char>(result[0]), target_val);
    ASSERT_EQ(result[0], expectedCharFromInt(inputValue));
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringMultipleValuesScaling) {
    OutOperator local_op(306);
    local_op.message(inputValueForChar('H'));
    local_op.message(inputValueForChar('e'));
    local_op.message(inputValueForChar('l'));
    local_op.message(inputValueForChar('l'));
    local_op.message(inputValueForChar('o'));

    std::string result = local_op.getDataAsString();
    ASSERT_EQ(result, "Hello");
    ASSERT_FALSE(local_op.hasOutput());
}

TEST_F(OutOperatorGetDataAsStringTests, GetDataAsStringHandlesNegativeValuesAsZeroChar) {
    OutOperator local_op(307);
    local_op.message(-100);
    std::string result = local_op.getDataAsString();
    ASSERT_EQ(result.length(), 1);
    ASSERT_EQ(static_cast<unsigned char>(result[0]), 0);
    ASSERT_EQ(result[0], expectedCharFromInt(-100));
    ASSERT_FALSE(local_op.hasOutput());

    OutOperator local_op_min(308);
    local_op_min.message(std::numeric_limits<int>::min());
    std::string result_min = local_op_min.getDataAsString();
    ASSERT_EQ(result_min.length(), 1);
    ASSERT_EQ(static_cast<unsigned char>(result_min[0]), 0);
    ASSERT_EQ(result_min[0], expectedCharFromInt(std::numeric_limits<int>::min()));
    ASSERT_FALSE(local_op_min.hasOutput());
}

TEST(OutOperatorHelperTests, ExpectedCharFromIntConsistency) {
    ASSERT_EQ(expectedCharFromInt(0), '\0');
    ASSERT_EQ(static_cast<unsigned char>(expectedCharFromInt(std::numeric_limits<int>::max())), 255);
    ASSERT_EQ(expectedCharFromInt(-10), '\0');
    ASSERT_EQ(expectedCharFromInt(inputValueForChar('A')), 'A');
    ASSERT_EQ(static_cast<unsigned char>(expectedCharFromInt(inputValueForChar(128))), 128);
    ASSERT_EQ(static_cast<unsigned char>(expectedCharFromInt(inputValueForChar(255))), 255);
}

class OutOperatorEqualityTests : public ::testing::Test {};

TEST_F(OutOperatorEqualityTests, IdenticalOutOperatorsAreEqual) {
    OutOperator op1(1);
    op1.message(10);
    op1.message(20);

    OutOperator op2(1);
    op2.message(10);
    op2.message(20);

    ASSERT_TRUE(op1 == op2);
    ASSERT_TRUE(op2 == op1);
    ASSERT_FALSE(op1 != op2);
}

TEST_F(OutOperatorEqualityTests, OutOperatorsWithDifferentDataAreNotEqual) {
    OutOperator op1(1);
    op1.message(10);
    op1.message(20);

    OutOperator op2(1);
    op2.message(10);
    op2.message(30);

    ASSERT_FALSE(op1 == op2);
    ASSERT_TRUE(op1 != op2);
}

TEST_F(OutOperatorEqualityTests, OutOperatorsWithDifferentDataOrderAreNotEqual) {
    OutOperator op1(1);
    op1.message(10);
    op1.message(20);

    OutOperator op2(1);
    op2.message(20);
    op2.message(10);

    ASSERT_FALSE(op1 == op2);
    ASSERT_TRUE(op1 != op2);
}

TEST_F(OutOperatorEqualityTests, OutOperatorsWithDifferentDataSizeAreNotEqual) {
    OutOperator op1(1);
    op1.message(10);
    op1.message(20);

    OutOperator op2(1);
    op2.message(10);

    ASSERT_FALSE(op1 == op2);
    ASSERT_TRUE(op1 != op2);

    OutOperator op3(1);
    op3.message(10);
    op3.message(20);
    op3.message(30);

    ASSERT_FALSE(op1 == op3);
    ASSERT_TRUE(op1 != op3);
}

TEST_F(OutOperatorEqualityTests, OutOperatorsWithDifferentIdsAreNotEqual) {
    OutOperator op1(1);
    op1.message(10);
    op1.message(20);

    OutOperator op2(2);
    op2.message(10);
    op2.message(20);

    ASSERT_FALSE(op1 == op2);
    ASSERT_TRUE(op1 != op2);
}

TEST_F(OutOperatorEqualityTests, OutOperatorNotEqualToOtherOperatorType) {
    OutOperator op_out(1);
    op_out.message(10);

    AddOperator op_add(1);

    const Operator& base_op_out = op_out;
    const Operator& base_op_add = op_add;

    ASSERT_FALSE(base_op_out == base_op_add);
    ASSERT_TRUE(base_op_out != base_op_add);
}

TEST_F(OutOperatorEqualityTests, EmptyOutOperatorsWithSameIdAreEqual) {
    OutOperator op1(1);
    OutOperator op2(1);

    ASSERT_TRUE(op1 == op2);
    ASSERT_FALSE(op1 != op2);
}

TEST_F(OutOperatorEqualityTests, EmptyOutOperatorsWithDifferentIdAreNotEqual) {
    OutOperator op1(1);
    OutOperator op2(2);

    ASSERT_FALSE(op1 == op2);
    ASSERT_TRUE(op1 != op2);
}

TEST_F(OutOperatorEqualityTests, OutOperatorEqualToSelf) {
    OutOperator op1(1);
    op1.message(50);
    op1.message(150);

    ASSERT_TRUE(op1 == op1);
    ASSERT_FALSE(op1 != op1);
}

TEST_F(OutOperatorEqualityTests, OutOperatorEqualToSelfWhenEmpty) {
    OutOperator op1(1);
    ASSERT_TRUE(op1 == op1);
    ASSERT_FALSE(op1 != op1);
}

class OutOperatorJsonTests : public ::testing::Test {
protected:
    // Helper lambda to read golden file and trim a single trailing newline
    std::function<std::string(const std::string&)> readAndTrimGolden = 
        [](const std::string& filePath) {
            std::string content = JsonTestHelpers::readGoldenFile(filePath);
            if (!content.empty() && content.back() == '\n') {
                content.pop_back();
            }
            return content;
    };
};

TEST_F(OutOperatorJsonTests, ToJsonEmptyDataMatchesGoldenFile) {
    // ARRANGE
    OutOperator op(123);
    std::string goldenOutput = readAndTrimGolden(MOCK_FILE_DIR + "out_operator_empty.json");

    // ACT: Call toJson with pretty-printing and enclosing brackets (defaults for this golden file)
    std::string actualOutput = op.toJson(true, true);

    // ASSERT: The output must exactly match the golden file
    EXPECT_EQ(actualOutput, goldenOutput);
}

TEST_F(OutOperatorJsonTests, ToJsonWithDataMatchesGoldenFile) {
    // ARRANGE
    OutOperator op(456);
    op.message(10);
    op.message(20);
    op.message(-5);
    std::string goldenOutput = readAndTrimGolden(MOCK_FILE_DIR + "out_operator_with_data.json");

    // ACT: Call toJson with pretty-printing and enclosing brackets (defaults for this golden file)
    std::string actualOutput = op.toJson(true, true);

    // ASSERT: The output must exactly match the golden file
    EXPECT_EQ(actualOutput, goldenOutput);
}

TEST_F(OutOperatorJsonTests, ToJsonPrettyPrintMatchesExistingGoldenFile) {
    // ARRANGE
    OutOperator op(789);
    op.message(100);
    // This golden file ('out_operator_pretty.json') is assumed to have a trailing newline,
    // as it was pre-existing and might be saved that way. The toJson output also has it.
    std::string goldenOutput = JsonTestHelpers::readGoldenFile(MOCK_FILE_DIR + "out_operator_pretty.json");
    // If toJson(true,true) for this case *doesn't* produce a trailing newline, 
    // then goldenOutput should also be trimmed. For now, assume it does.
    // std::string actualOutput = op.toJson(true, true);
    // if (!actualOutput.empty() && actualOutput.back() == '\n' && 
    //     !goldenOutput.empty() && goldenOutput.back() != '\n') {
    //      // This should not happen if actual includes newline and golden does not
    // }
    // if (!goldenOutput.empty() && goldenOutput.back() == '\n' &&
    //     !actualOutput.empty() && actualOutput.back() != '\n') {
    //     goldenOutput.pop_back(); // Trim golden if actual doesn't have newline
    // }


    // ACT
    std::string actualOutput = op.toJson(true, true);

    // ASSERT: Perform a single, strict comparison.
    // Let's check if the pre-existing golden file has a newline the actual doesn't, or vice-versa
    std::string adjustedGoldenOutput = goldenOutput;
    if (!actualOutput.empty() && actualOutput.back() == '\n' && !adjustedGoldenOutput.empty() && adjustedGoldenOutput.back() != '\n') {
        // This case is fine, actual has newline, golden doesn't - test will fail if not equal.
        // But if golden has one and actual doesn't, we might need to trim golden.
    } else if (!adjustedGoldenOutput.empty() && adjustedGoldenOutput.back() == '\n' && (actualOutput.empty() || actualOutput.back() != '\n')) {
        adjustedGoldenOutput.pop_back();
    }


    EXPECT_EQ(actualOutput, adjustedGoldenOutput);
}

TEST_F(OutOperatorJsonTests, ToJsonNoEnclosingBrackets) { // Renamed
    // ARRANGE
    OutOperator op(12);
    op.message(100);

    // Manually define the expected fragment, matching op.toJson(true, false)'s output
    // This is done because of issues with `overwrite_file_with_block` not preserving
    // leading whitespace on the first line of a block for the golden file.
    std::string expectedOutputFragment = 
        "  \"opType\": \"OUT\",\n"
        "  \"operatorId\": 12,\n"
        "  \"outputDistanceBuckets\": [],\n"
        "  \"data\": [\n"
        "    100\n"
        "  ]";

    // ACT
    std::string actualOutputFragment = op.toJson(true, false); // pretty, no brackets

    // ASSERT
    EXPECT_EQ(actualOutputFragment, expectedOutputFragment);

    // Additionally, check the compact form without brackets
    OutOperator op_compact(13);
    op_compact.message(200);
    // Expected compact fragment (manually constructed for clarity, as golden files are typically pretty-printed)
    // Note: Operator::toJson(false, false) produces the base class part.
    // OutOperator::toJson appends its "data" part.
    // Base part for op_compact(13): "\"opType\":\"OUT\",\"operatorId\":13,\"outputDistanceBuckets\":[]"
    // Appended part: ",\"data\":[200]"
    std::string expectedCompactFragment = R"#("opType":"OUT","operatorId":13,"outputDistanceBuckets":[],"data":[200])#";
    std::string actualCompactFragment = op_compact.toJson(false, false); // not pretty, no brackets
    EXPECT_EQ(actualCompactFragment, expectedCompactFragment);
}

TEST_F(OutOperatorJsonTests, ToJsonEnclosedBracketsCompactMatchesManual) {
    // ARRANGE
    OutOperator op(14);
    op.message(300);
    op.message(400);

    // Manually construct the expected compact JSON string with enclosing brackets
    std::string expectedCompactJson = R"#({"opType":"OUT","operatorId":14,"outputDistanceBuckets":[],"data":[300,400]})#";
    
    // ACT
    std::string actualCompactJson = op.toJson(false, true); // not pretty, with brackets

    // ASSERT
    EXPECT_EQ(actualCompactJson, expectedCompactJson);
}


// --- Tests for Serialization ---
class OutOperatorSerializationTests : public ::testing::Test {};

TEST_F(OutOperatorSerializationTests, SerializeDeserializeEmptyData) {
    OutOperator op1(123);

    std::vector<std::byte> bytes = op1.serializeToBytes();

    const std::byte* current_ptr = bytes.data();
    const std::byte* stream_end = bytes.data() + bytes.size();
    
    // Step 1: Read the total size prefix (advances pointer by 4)
    uint32_t total_data_size = Serializer::read_uint32(current_ptr, stream_end);
    ASSERT_EQ(total_data_size, bytes.size() - 4);

    // Step 2 (CRITICAL FIX): Read the opType from the stream (advances pointer by 2)
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current_ptr, stream_end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::OUT);

    // 'current_ptr' now correctly points to the start of the operatorId.
    const std::byte* operator_data_block_end = bytes.data() + 4 + total_data_size;
    
    // Step 3: Call the constructor with the correctly positioned pointer.
    OutOperator op2(current_ptr, operator_data_block_end);

    ASSERT_TRUE(op1 == op2);
    ASSERT_EQ(current_ptr, operator_data_block_end) << "Not all bytes consumed during deserialization.";
}

TEST_F(OutOperatorSerializationTests, SerializeDeserializeWithData) {
    OutOperator op1(456);
    op1.message(10);
    op1.message(0);
    op1.message(-25);

    std::vector<std::byte> bytes = op1.serializeToBytes();

    ASSERT_GE(bytes.size(), 4u);
    const std::byte* current_ptr = bytes.data();
    const std::byte* stream_end = bytes.data() + bytes.size();

    // Step 1: Read the total size prefix (advances pointer by 4)
    uint32_t total_data_size = Serializer::read_uint32(current_ptr, stream_end);
    ASSERT_EQ(total_data_size, bytes.size() - 4);

    // Step 2 (CRITICAL FIX): Read the opType from the stream (advances pointer by 2)
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current_ptr, stream_end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::OUT);

    // 'current_ptr' now correctly points to the start of the operatorId.
    const std::byte* operator_data_block_end = bytes.data() + 4 + total_data_size;
    
    // Step 3: Call the constructor with the correctly positioned pointer.
    OutOperator op2(current_ptr, operator_data_block_end);

    ASSERT_TRUE(op1 == op2);
    ASSERT_EQ(current_ptr, operator_data_block_end) << "Not all bytes consumed during deserialization.";
}

TEST_F(OutOperatorSerializationTests, SerializeDeserializeMaxMinIntData) {
    // ARRANGE: Create an operator with data at the limits of the 'int' type.
    OutOperator op1(789);
    op1.message(std::numeric_limits<int>::max());
    op1.message(std::numeric_limits<int>::min());
    op1.message(0);

    // ACT: Serialize the operator to a byte vector.
    std::vector<std::byte> bytes = op1.serializeToBytes();

    // --- Start of Corrected Deserialization Logic ---

    ASSERT_GE(bytes.size(), 4u);
    const std::byte* current_ptr = bytes.data();
    const std::byte* stream_end = current_ptr + bytes.size();

    // 1. Simulate a controller reading the 4-byte total size prefix.
    uint32_t total_data_size = Serializer::read_uint32(current_ptr, stream_end);
    ASSERT_EQ(total_data_size, bytes.size() - 4);

    // 2. CRITICAL FIX: Simulate the controller reading the 2-byte Operator::Type
    //    to know which constructor to call. This advances the pointer correctly.
    Operator::Type op_type_from_stream = static_cast<Operator::Type>(Serializer::read_uint16(current_ptr, stream_end));
    ASSERT_EQ(op_type_from_stream, Operator::Type::OUT);

    // The 'current_ptr' now correctly points to the start of the Operator ID field.
    const std::byte* operator_data_block_end = bytes.data() + 4 + total_data_size;

    // 3. Call the deserialization constructor with the correctly positioned pointer.
    OutOperator op2(current_ptr, operator_data_block_end);

    // --- End of Corrected Deserialization Logic ---


    // ASSERT: The original and deserialized objects are identical.
    ASSERT_TRUE(op1 == op2);
    // Assert that all bytes in the data block were consumed by the constructor.
    ASSERT_EQ(current_ptr, operator_data_block_end) << "Not all bytes were consumed during deserialization.";
}

// --- Tests for Constructor (ID) ---
class OutOperatorConstructorTests : public ::testing::Test {};

TEST_F(OutOperatorConstructorTests, DefaultConstructorInitializesCorrectly) {
    uint32_t test_id = 987;
    OutOperator op(test_id);

    ASSERT_EQ(op.getId(), static_cast<int>(test_id));
    ASSERT_EQ(op.getOpType(), Operator::Type::OUT);
    ASSERT_FALSE(op.hasOutput());

    std::string json_str = op.toJson();

    ASSERT_EQ(JsonTestHelpers::getJsonIntValue(json_str, "operatorId"), static_cast<int>(test_id));
    ASSERT_NE(json_str.find("\"opType\":\"" + Operator::typeToString(Operator::Type::OUT) + "\""), std::string::npos);

    std::vector<int> data_array = JsonTestHelpers::getJsonArrayIntValue(json_str, "data");
    ASSERT_TRUE(data_array.empty());

    ASSERT_NE(json_str.find("\"outputDistanceBuckets\":[]"), std::string::npos);
}

// --- Tests for randomInit ---
class OutOperatorRandomInitTests : public ::testing::Test {};

TEST_F(OutOperatorRandomInitTests, RandomInitMaxIdDoesNotAddConnections) {
    OutOperator op(123);
    MockRandomizer mr;
    uint32_t initial_id = op.getId();

    std::string initial_json_str = op.toJson();
    std::vector<int> initial_data_array = JsonTestHelpers::getJsonArrayIntValue(initial_json_str, "data");

    op.randomInit(100u, &mr);

    std::string final_json_str = op.toJson();

    ASSERT_EQ(JsonTestHelpers::getJsonIntValue(final_json_str, "operatorId"), static_cast<int>(initial_id));
    ASSERT_NE(final_json_str.find("\"opType\":\"" + Operator::typeToString(Operator::Type::OUT) + "\""), std::string::npos);
    ASSERT_NE(final_json_str.find("\"outputDistanceBuckets\":[]"), std::string::npos) << "outputDistanceBuckets should be empty after randomInit(maxId). JSON: " << final_json_str;

    std::vector<int> final_data_array = JsonTestHelpers::getJsonArrayIntValue(final_json_str, "data");
    ASSERT_EQ(initial_data_array, final_data_array) << "Data array should not change.";
    ASSERT_TRUE(final_data_array.empty()) << "Data array should remain empty.";
}

TEST_F(OutOperatorRandomInitTests, RandomInitIdRangeDoesNotAddConnections) {
    OutOperator op(456);
    MockRandomizer mr;
    IdRange id_r(0, 0);
    uint32_t initial_id = op.getId();

    std::string initial_json_str = op.toJson();
    std::vector<int> initial_data_array = JsonTestHelpers::getJsonArrayIntValue(initial_json_str, "data");

    op.randomInit(&id_r, &mr);

    std::string final_json_str = op.toJson();

    ASSERT_EQ(JsonTestHelpers::getJsonIntValue(final_json_str, "operatorId"), static_cast<int>(initial_id));
    ASSERT_NE(final_json_str.find("\"opType\":\"" + Operator::typeToString(Operator::Type::OUT) + "\""), std::string::npos);
    ASSERT_NE(final_json_str.find("\"outputDistanceBuckets\":[]"), std::string::npos) << "outputDistanceBuckets should be empty after randomInit(IdRange). JSON: " << final_json_str;

    std::vector<int> final_data_array = JsonTestHelpers::getJsonArrayIntValue(final_json_str, "data");
    ASSERT_EQ(initial_data_array, final_data_array) << "Data array should not change.";
    ASSERT_TRUE(final_data_array.empty()) << "Data array should remain empty.";
}
