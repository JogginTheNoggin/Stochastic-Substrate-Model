#include <gtest/gtest.h>
#include "operators/OutOperator.h" // Adjust path as necessary based on include paths in CMake
#include "operators/AddOperator.h"   // For cross-type equality test
#include "helpers/JsonTestHelpers.h" // For JSON comparison
#include "helpers/MockRandomizer.h" // For randomInit tests
#include "util/Serializer.h"       // For serialization tests
#include <limits> // Required for std::numeric_limits
#include <cmath>  // Required for std::round, std::isnan, std::isinf (though indirectly via OutOperator)
#include <string> // Required for std::string
#include <vector> // Required for std::vector
#include <cstddef> // Required for std::byte
#include "nlohmann/json.hpp" // For nlohmann::json type in helper

// Anonymous namespace for helper functions specific to this test file
namespace {
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

    nlohmann::json getOperatorDataJson(OutOperator& operator_instance) {
        // toJson(prettyPrint=false, encloseInBrackets=true) is the default for toJson()
        auto json_obj = JsonTestHelpers::parseJson(operator_instance.toJson(false, true));
        if (json_obj.contains("data") && json_obj["data"].is_array()) {
            return json_obj["data"];
        }
        return nlohmann::json::array();
    }
};

// Test suite for message handling specifically
class OutOperatorMessageTests : public OutOperatorTest {
};

TEST_F(OutOperatorMessageTests, HandlesIntMessages) {
    OutOperator local_op(1);
    local_op.message(10);
    local_op.message(-5);
    local_op.message(0);

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 3);
    ASSERT_EQ(data_array[0].get<int>(), 10);
    ASSERT_EQ(data_array[1].get<int>(), -5);
    ASSERT_EQ(data_array[2].get<int>(), 0);

    OutOperator op_max_min(2);
    op_max_min.message(std::numeric_limits<int>::max());
    op_max_min.message(std::numeric_limits<int>::min());

    data_array = getOperatorDataJson(op_max_min);
    ASSERT_EQ(data_array.size(), 2);
    ASSERT_EQ(data_array[0].get<int>(), std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[1].get<int>(), std::numeric_limits<int>::min());
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

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 7);
    ASSERT_EQ(data_array[0].get<int>(), 10);
    ASSERT_EQ(data_array[1].get<int>(), 11);
    ASSERT_EQ(data_array[2].get<int>(), -5);
    ASSERT_EQ(data_array[3].get<int>(), -6);
    ASSERT_EQ(data_array[4].get<int>(), 0);
    ASSERT_EQ(data_array[5].get<int>(), 0);
    ASSERT_EQ(data_array[6].get<int>(), 1);
}

TEST_F(OutOperatorMessageTests, HandlesFloatMessagesClamping) {
    OutOperator local_op(1);
    local_op.message(std::numeric_limits<float>::max());
    local_op.message(std::numeric_limits<float>::lowest());
    local_op.message(static_cast<float>(std::numeric_limits<int>::max()) + 100.0f);
    local_op.message(static_cast<float>(std::numeric_limits<int>::min()) - 100.0f);

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 4);
    ASSERT_EQ(data_array[0].get<int>(), std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[1].get<int>(), std::numeric_limits<int>::min());
    ASSERT_EQ(data_array[2].get<int>(), std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[3].get<int>(), std::numeric_limits<int>::min());
}

TEST_F(OutOperatorMessageTests, HandlesFloatMessagesSpecialValues) {
    OutOperator local_op(1);
    local_op.message(1);
    local_op.message(std::numeric_limits<float>::quiet_NaN());
    local_op.message(std::numeric_limits<float>::infinity());
    local_op.message(-std::numeric_limits<float>::infinity());

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 1);
    ASSERT_EQ(data_array[0].get<int>(), 1);
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

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 7);
    ASSERT_EQ(data_array[0].get<int>(), 20);
    ASSERT_EQ(data_array[1].get<int>(), 21);
    ASSERT_EQ(data_array[2].get<int>(), -15);
    ASSERT_EQ(data_array[3].get<int>(), -16);
    ASSERT_EQ(data_array[4].get<int>(), 0);
    ASSERT_EQ(data_array[5].get<int>(), 0);
    ASSERT_EQ(data_array[6].get<int>(), 1);
}

TEST_F(OutOperatorMessageTests, HandlesDoubleMessagesClamping) {
    OutOperator local_op(1);
    local_op.message(std::numeric_limits<double>::max());
    local_op.message(std::numeric_limits<double>::lowest());
    local_op.message(static_cast<double>(std::numeric_limits<int>::max()) + 100.0);
    local_op.message(static_cast<double>(std::numeric_limits<int>::min()) - 100.0);

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 4);
    ASSERT_EQ(data_array[0].get<int>(), std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[1].get<int>(), std::numeric_limits<int>::min());
    ASSERT_EQ(data_array[2].get<int>(), std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[3].get<int>(), std::numeric_limits<int>::min());
}

TEST_F(OutOperatorMessageTests, HandlesDoubleMessagesSpecialValues) {
    OutOperator local_op(1);
    local_op.message(2);
    local_op.message(std::numeric_limits<double>::quiet_NaN());
    local_op.message(std::numeric_limits<double>::infinity());
    local_op.message(-std::numeric_limits<double>::infinity());

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 1);
    ASSERT_EQ(data_array[0].get<int>(), 2);
}

TEST_F(OutOperatorMessageTests, HandlesMixedTypeMessages) {
    OutOperator local_op(1);
    local_op.message(100);
    local_op.message(20.7f);
    local_op.message(-30.3);
    local_op.message(std::numeric_limits<int>::max());
    local_op.message(0.4f);
    local_op.message(static_cast<double>(std::numeric_limits<int>::min()) - 2000.0);

    auto data_array = getOperatorDataJson(local_op);
    ASSERT_EQ(data_array.size(), 6);
    ASSERT_EQ(data_array[0].get<int>(), 100);
    ASSERT_EQ(data_array[1].get<int>(), 21);
    ASSERT_EQ(data_array[2].get<int>(), -30);
    ASSERT_EQ(data_array[3].get<int>(), std::numeric_limits<int>::max());
    ASSERT_EQ(data_array[4].get<int>(), 0);
    ASSERT_EQ(data_array[5].get<int>(), std::numeric_limits<int>::min());
}

TEST_F(OutOperatorTest, EmptyOperatorToJsonCheck) {
    OutOperator local_op(1);
    std::string json_str = local_op.toJson();
    nlohmann::json actual_json_obj = JsonTestHelpers::parseJson(json_str);

    ASSERT_TRUE(actual_json_obj.contains("operatorId"));
    ASSERT_EQ(actual_json_obj["operatorId"].get<uint32_t>(), 1);

    ASSERT_TRUE(actual_json_obj.contains("opType"));
    ASSERT_EQ(actual_json_obj["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));

    ASSERT_TRUE(actual_json_obj.contains("outputConnections"));
    ASSERT_TRUE(actual_json_obj["outputConnections"].is_array());
    ASSERT_TRUE(actual_json_obj["outputConnections"].empty());

    ASSERT_TRUE(actual_json_obj.contains("data"));
    ASSERT_TRUE(actual_json_obj["data"].is_array());
    ASSERT_TRUE(actual_json_obj["data"].empty());
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

    nlohmann::json data_before = getOperatorDataJson(local_op);
    bool has_output_before = local_op.hasOutput();

    local_op.processData();

    nlohmann::json data_after = getOperatorDataJson(local_op);
    bool has_output_after = local_op.hasOutput();

    ASSERT_EQ(data_before, data_after);
    ASSERT_EQ(has_output_before, has_output_after);
    ASSERT_TRUE(has_output_after);
    ASSERT_EQ(data_after.size(), 2);
    ASSERT_EQ(data_after[0].get<int>(), 10);
    ASSERT_EQ(data_after[1].get<int>(), 21);
}

TEST_F(OutOperatorTest, ProcessDataOnEmptyBufferHasNoEffect) {
    OutOperator local_op(201);
    ASSERT_FALSE(local_op.hasOutput());

    nlohmann::json data_before = getOperatorDataJson(local_op);

    local_op.processData();

    nlohmann::json data_after = getOperatorDataJson(local_op);
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

class OutOperatorJsonTests : public ::testing::Test {};

TEST_F(OutOperatorJsonTests, ToJsonSchemaAndContentEmptyData) {
    OutOperator op(123);
    std::string json_str = op.toJson();
    nlohmann::json parsed_json = JsonTestHelpers::parseJson(json_str);

    ASSERT_TRUE(parsed_json.contains("operatorId"));
    ASSERT_EQ(parsed_json["operatorId"].get<uint32_t>(), 123);

    ASSERT_TRUE(parsed_json.contains("opType"));
    ASSERT_EQ(parsed_json["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));

    ASSERT_TRUE(parsed_json.contains("outputConnections"));
    ASSERT_TRUE(parsed_json["outputConnections"].is_array());
    ASSERT_TRUE(parsed_json["outputConnections"].empty());

    ASSERT_TRUE(parsed_json.contains("data"));
    ASSERT_TRUE(parsed_json["data"].is_array());
    ASSERT_TRUE(parsed_json["data"].empty());
}

TEST_F(OutOperatorJsonTests, ToJsonSchemaAndContentWithData) {
    OutOperator op(456);
    op.message(10);
    op.message(20);
    op.message(-5);
    std::string json_str = op.toJson();
    nlohmann::json parsed_json = JsonTestHelpers::parseJson(json_str);

    ASSERT_TRUE(parsed_json.contains("operatorId"));
    ASSERT_EQ(parsed_json["operatorId"].get<uint32_t>(), 456);

    ASSERT_TRUE(parsed_json.contains("opType"));
    ASSERT_EQ(parsed_json["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));

    ASSERT_TRUE(parsed_json.contains("outputConnections"));
    ASSERT_TRUE(parsed_json["outputConnections"].is_array());
    ASSERT_TRUE(parsed_json["outputConnections"].empty());

    ASSERT_TRUE(parsed_json.contains("data"));
    ASSERT_TRUE(parsed_json["data"].is_array());
    ASSERT_EQ(parsed_json["data"].size(), 3);
    ASSERT_EQ(parsed_json["data"][0].get<int>(), 10);
    ASSERT_EQ(parsed_json["data"][1].get<int>(), 20);
    ASSERT_EQ(parsed_json["data"][2].get<int>(), -5);
}

TEST_F(OutOperatorJsonTests, ToJsonPrettyPrintOption) {
    OutOperator op(789);
    op.message(100);

    std::string json_pretty_str = op.toJson(true, true);
    std::string json_compact_str = op.toJson(false, true);

    ASSERT_NE(json_pretty_str.find('\n'), std::string::npos);
    ASSERT_NE(json_pretty_str.find("  "), std::string::npos);

    nlohmann::json parsed_pretty = JsonTestHelpers::parseJson(json_pretty_str);
    nlohmann::json parsed_compact = JsonTestHelpers::parseJson(json_compact_str);

    ASSERT_EQ(parsed_pretty["operatorId"].get<uint32_t>(), 789);
    ASSERT_EQ(parsed_compact["operatorId"].get<uint32_t>(), 789);
    ASSERT_EQ(parsed_pretty["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));
    ASSERT_EQ(parsed_compact["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));
    ASSERT_TRUE(parsed_pretty["outputConnections"].empty());
    ASSERT_TRUE(parsed_compact["outputConnections"].empty());
    ASSERT_EQ(parsed_pretty["data"][0].get<int>(), 100);
    ASSERT_EQ(parsed_compact["data"][0].get<int>(), 100);

    ASSERT_EQ(parsed_pretty, parsed_compact);
}

TEST_F(OutOperatorJsonTests, ToJsonEncloseInBracketsOption) {
    OutOperator op(12);
    op.message(50);

    std::string json_enclosed_str = op.toJson(false, true);
    std::string json_not_enclosed_str = op.toJson(false, false);

    ASSERT_FALSE(json_enclosed_str.empty());
    ASSERT_TRUE(json_enclosed_str.rfind('{', 0) == 0);
    ASSERT_EQ(json_enclosed_str.back(), '}');

    ASSERT_FALSE(json_not_enclosed_str.empty());
    ASSERT_NE(json_not_enclosed_str.find('{', 0), 0);
    ASSERT_NE(json_not_enclosed_str.back(), '}');

    nlohmann::json parsed_enclosed = JsonTestHelpers::parseJson(json_enclosed_str);
    ASSERT_TRUE(parsed_enclosed.contains("operatorId"));
    ASSERT_EQ(parsed_enclosed["operatorId"].get<uint32_t>(), 12);
    ASSERT_TRUE(parsed_enclosed.contains("opType"));
    ASSERT_EQ(parsed_enclosed["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));
    ASSERT_TRUE(parsed_enclosed.contains("outputConnections"));
    ASSERT_TRUE(parsed_enclosed["outputConnections"].is_array());
    ASSERT_TRUE(parsed_enclosed["outputConnections"].empty());
    ASSERT_TRUE(parsed_enclosed.contains("data"));
    ASSERT_EQ(parsed_enclosed["data"][0].get<int>(), 50);

    std::string temp_valid_json_from_fragment = "{" + json_not_enclosed_str + "}";
    nlohmann::json parsed_fragment = JsonTestHelpers::parseJson(temp_valid_json_from_fragment);
    ASSERT_TRUE(parsed_fragment.contains("operatorId"));
    ASSERT_EQ(parsed_fragment["operatorId"].get<uint32_t>(), 12);
    ASSERT_TRUE(parsed_fragment.contains("opType"));
    ASSERT_EQ(parsed_fragment["opType"].get<std::string>(), Operator::typeToString(Operator::Type::OUT));
    ASSERT_TRUE(parsed_fragment.contains("outputConnections"));
    ASSERT_TRUE(parsed_fragment["outputConnections"].is_array());
    ASSERT_TRUE(parsed_fragment["outputConnections"].empty());
    ASSERT_TRUE(parsed_fragment.contains("data"));
    ASSERT_EQ(parsed_fragment["data"][0].get<int>(), 50);

    ASSERT_EQ(parsed_enclosed, parsed_fragment);
}

// --- Tests for Serialization ---
class OutOperatorSerializationTests : public ::testing::Test {};

TEST_F(OutOperatorSerializationTests, SerializeDeserializeEmptyData) {
    OutOperator op1(123);

    std::vector<std::byte> bytes = op1.serializeToBytes();

    ASSERT_GE(bytes.size(), 4u); // Minimum size for the prefix
    const std::byte* current_for_prefix_check = bytes.data();
    // Temporarily advance a pointer to read prefix without affecting actual deserialization start
    const std::byte* end_for_prefix_check = bytes.data() + bytes.size();
    uint32_t deserialized_size_prefix = Serializer::read_uint32(current_for_prefix_check, end_for_prefix_check);
    ASSERT_EQ(deserialized_size_prefix, bytes.size() - 4);

    const std::byte* payload_start = bytes.data() + 4; // Actual payload starts after the size prefix
    const std::byte* stream_end = bytes.data() + bytes.size();

    const std::byte* current_for_deserialization = payload_start; // This pointer will be advanced by constructor
    OutOperator op2(current_for_deserialization, stream_end);

    ASSERT_TRUE(op1 == op2);
    ASSERT_EQ(current_for_deserialization, stream_end) << "Not all bytes consumed during deserialization.";
}

TEST_F(OutOperatorSerializationTests, SerializeDeserializeWithData) {
    OutOperator op1(456);
    op1.message(10);
    op1.message(0);
    op1.message(-25);

    std::vector<std::byte> bytes = op1.serializeToBytes();

    ASSERT_GE(bytes.size(), 4u);
    const std::byte* current_for_prefix_check = bytes.data();
    const std::byte* end_for_prefix_check = bytes.data() + bytes.size();
    uint32_t deserialized_size_prefix = Serializer::read_uint32(current_for_prefix_check, end_for_prefix_check);
    ASSERT_EQ(deserialized_size_prefix, bytes.size() - 4);

    const std::byte* payload_start = bytes.data() + 4;
    const std::byte* stream_end = bytes.data() + bytes.size();

    const std::byte* current_for_deserialization = payload_start;
    OutOperator op2(current_for_deserialization, stream_end);

    ASSERT_TRUE(op1 == op2);
    ASSERT_EQ(current_for_deserialization, stream_end) << "Not all bytes consumed.";
}

TEST_F(OutOperatorSerializationTests, SerializeDeserializeMaxMinIntData) {
    OutOperator op1(789);
    op1.message(std::numeric_limits<int>::max());
    op1.message(std::numeric_limits<int>::min());
    op1.message(0); // Add a zero too for variety

    std::vector<std::byte> bytes = op1.serializeToBytes();

    ASSERT_GE(bytes.size(), 4u);
    const std::byte* current_for_prefix_check = bytes.data();
    const std::byte* end_for_prefix_check = bytes.data() + bytes.size();
    uint32_t deserialized_size_prefix = Serializer::read_uint32(current_for_prefix_check, end_for_prefix_check);
    ASSERT_EQ(deserialized_size_prefix, bytes.size() - 4);

    const std::byte* payload_start = bytes.data() + 4;
    const std::byte* stream_end = bytes.data() + bytes.size();

    const std::byte* current_for_deserialization = payload_start;
    OutOperator op2(current_for_deserialization, stream_end);

    ASSERT_TRUE(op1 == op2);
    ASSERT_EQ(current_for_deserialization, stream_end) << "Not all bytes consumed.";
}
