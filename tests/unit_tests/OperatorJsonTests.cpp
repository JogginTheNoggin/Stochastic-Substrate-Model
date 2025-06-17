#include "gtest/gtest.h"
#include "headers/operators/AddOperator.h" // Using AddOperator to test Operator's toJson
#include "headers/operators/Operator.h"
#include <memory>
#include <string>
#include <vector>       // For std::vector<int> in requestUpdate (if used by toJson indirectly, though unlikely)
#include <unordered_set> // For outputConnections
#include <algorithm>    // For std::remove_if
#include <cctype>       // For ::isspace

// Fixture for Operator toJson tests
class OperatorJsonTest : public ::testing::Test {
protected:
    // No complex setup needed for toJson
    void SetUp() override {}
    void TearDown() override {}

    // Helper to remove all whitespace from a string for easier comparison
    std::string removeWhitespace(std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
        return s;
    }
};

TEST_F(OperatorJsonTest, ToJsonNoConnections) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(123); // Op ID 123

    // AddOperator::toJson appends its own fields (weight, threshold, accumulateData)
    // after calling Operator::toJson. So expected JSON needs to include these.
    // Default AddOperator constructor: weight=1, threshold=0, accumulateData=0
    std::string expectedJsonNoPretty =
        "{"
        "\"opType\":\"ADD\","
        "\"operatorId\":123,"
        "\"outputDistanceBuckets\":[],"  // End of Operator part
        "\"weight\":1,"
        "\"threshold\":0,"
        "\"accumulateData\":0"
        "}";

    std::string actualJsonNoPretty = op->toJson(false, true);
    EXPECT_EQ(removeWhitespace(actualJsonNoPretty), removeWhitespace(expectedJsonNoPretty));

    std::string expectedJsonPretty =
        "{\n"
        "  \"opType\": \"ADD\",\n"
        "  \"operatorId\": 123,\n"
        "  \"outputDistanceBuckets\": [],\n" // End of Operator part for pretty
        "  \"weight\": 1,\n"
        "  \"threshold\": 0,\n"
        "  \"accumulateData\": 0\n"
        "}";
    std::string actualJsonPretty = op->toJson(true, true);
    EXPECT_EQ(removeWhitespace(actualJsonPretty), removeWhitespace(expectedJsonPretty));
}

TEST_F(OperatorJsonTest, ToJsonOneConnectionOneDistance) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(124); // weight=1, threshold=0
    op->addConnectionInternal(200, 2); // Target 200, distance 2

    std::string expectedBaseJsonNoPretty =
        "{"
        "\"opType\":\"ADD\","
        "\"operatorId\":124,"
        "\"outputDistanceBuckets\":["
        "  {"
        "  \"distance\":2,"
        "  \"targetOperatorIds\":[200]"
        "  }"
        "]," // Comma after Operator part
        "\"weight\":1,"
        "\"threshold\":0,"
        "\"accumulateData\":0"
        "}";
    std::string actualJsonNoPretty = op->toJson(false, true);
    EXPECT_EQ(removeWhitespace(actualJsonNoPretty), removeWhitespace(expectedBaseJsonNoPretty));

    std::string actualJsonPretty = op->toJson(true, true);
    EXPECT_NE(actualJsonPretty.find("\"distance\": 2"), std::string::npos);
    EXPECT_NE(actualJsonPretty.find("\"targetOperatorIds\": [200]"), std::string::npos);
    EXPECT_NE(actualJsonPretty.find("\"weight\": 1"), std::string::npos);
    EXPECT_EQ(removeWhitespace(actualJsonPretty), removeWhitespace(expectedBaseJsonNoPretty));
}

TEST_F(OperatorJsonTest, ToJsonMultipleConnectionsOneDistance) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(125); // weight=1, threshold=0
    op->addConnectionInternal(300, 1);
    op->addConnectionInternal(301, 1);

    std::string actualJsonNoPretty = op->toJson(false, true);
    std::string actualJsonPretty = op->toJson(true, true);

    EXPECT_NE(actualJsonNoPretty.find("\"distance\":1"), std::string::npos);
    bool foundTargetsNoPretty = (actualJsonNoPretty.find("300") != std::string::npos && actualJsonNoPretty.find("301") != std::string::npos);
    EXPECT_TRUE(foundTargetsNoPretty);
    // Check structure without specific order of 300,301 in array
    std::string expectedStructureNoPretty = removeWhitespace(
        "{\"opType\":\"ADD\",\"operatorId\":125,\"outputDistanceBuckets\":[{\"distance\":1,\"targetOperatorIds\":[__TARGETS__]}],\"weight\":1,\"threshold\":0,\"accumulateData\":0}"
    );
    std::string actualStrippedNoPretty = removeWhitespace(actualJsonNoPretty);
    // Simple check: ensure all components are there
    EXPECT_TRUE(actualStrippedNoPretty.rfind(removeWhitespace("{\"opType\":\"ADD\",\"operatorId\":125,")) == 0); // Starts with
    EXPECT_NE(actualStrippedNoPretty.find(removeWhitespace("\"outputDistanceBuckets\":[{\"distance\":1,\"targetOperatorIds\":[")), std::string::npos);
    EXPECT_NE(actualStrippedNoPretty.find("300"), std::string::npos);
    EXPECT_NE(actualStrippedNoPretty.find("301"), std::string::npos);
    EXPECT_TRUE(actualStrippedNoPretty.rfind(removeWhitespace("]}],\"weight\":1,\"threshold\":0,\"accumulateData\":0}")) != std::string::npos);


    EXPECT_NE(actualJsonPretty.find("\"distance\": 1"), std::string::npos);
    bool foundTargetsPretty = (actualJsonPretty.find("300") != std::string::npos && actualJsonPretty.find("301") != std::string::npos);
    EXPECT_TRUE(foundTargetsPretty);
    EXPECT_NE(actualJsonPretty.find("\"weight\": 1"), std::string::npos);
}

TEST_F(OperatorJsonTest, ToJsonMultipleConnectionsMultipleDistances) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(126); // weight=1, threshold=0
    op->addConnectionInternal(400, 0);
    op->addConnectionInternal(401, 3);
    op->addConnectionInternal(402, 3);

    std::string actualJson = op->toJson(true, true);

    EXPECT_NE(actualJson.find("\"opType\": \"ADD\""), std::string::npos);
    EXPECT_NE(actualJson.find("\"operatorId\": 126"), std::string::npos);
    EXPECT_NE(actualJson.find("\"weight\": 1"), std::string::npos);

    EXPECT_NE(actualJson.find("\"distance\": 0"), std::string::npos);
    EXPECT_NE(actualJson.find("\"targetOperatorIds\": [400]"), std::string::npos);

    EXPECT_NE(actualJson.find("\"distance\": 3"), std::string::npos);
    bool foundTargetsDist3 = (actualJson.find("401") != std::string::npos && actualJson.find("402") != std::string::npos);
    EXPECT_TRUE(foundTargetsDist3);
}

TEST_F(OperatorJsonTest, ToJsonNoEnclosingBrackets) {
    // Tests AddOperator::toJson(pretty, false)
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(127); // weight=1, threshold=0
    op->addConnectionInternal(500, 1);

    std::string jsonWithoutBrackets = op->toJson(false, false); // pretty=false, encloseInBrackets=false

    // Expected Operator part: "\"opType\":\"ADD\",\"operatorId\":127,\"outputDistanceBuckets\":[{\"distance\":1,\"targetOperatorIds\":[500]}]"
    // Expected AddOperator part (appended): ",\"weight\":1,\"threshold\":0,\"accumulateData\":0"
    // Combined:
    std::string expectedContent =
        "\"opType\":\"ADD\","
        "\"operatorId\":127,"
        "\"outputDistanceBuckets\":["
        "  {\"distance\":1,\"targetOperatorIds\":[500]}"
        "]," // Comma from AddOperator::toJson separating base and derived parts
        "\"weight\":1,"
        "\"threshold\":0,"
        "\"accumulateData\":0";

    EXPECT_EQ(removeWhitespace(jsonWithoutBrackets), removeWhitespace(expectedContent));

    // Check that it does NOT start/end with { }
    if (!jsonWithoutBrackets.empty()) {
        EXPECT_NE(jsonWithoutBrackets.front(), '{');
        EXPECT_NE(jsonWithoutBrackets.back(), '}');
    }

    std::string jsonWithoutBracketsPretty = op->toJson(true, false); // pretty=true, encloseInBrackets=false
    // Operator::toJson(true, false) output:
    // "  "opType": "ADD",
    // "  "operatorId": 127,
    // "  "outputDistanceBuckets": [
    // "    {
    // "      "distance": 1,
    // "      "targetOperatorIds": [500]
    // "    }
    // "  ]"
    // AddOperator::toJson appends: ",\n  "weight": 1, ... " (indent assumed to be "  ")
    EXPECT_NE(jsonWithoutBracketsPretty.find("\"opType\": \"ADD\","), std::string::npos);
    EXPECT_NE(jsonWithoutBracketsPretty.find("\"targetOperatorIds\": [500]"), std::string::npos);
    EXPECT_NE(jsonWithoutBracketsPretty.find(",\n  \"weight\": 1"), std::string::npos); // Check for AddOperator part with pretty print
    if (!jsonWithoutBracketsPretty.empty()) {
        EXPECT_NE(jsonWithoutBracketsPretty.front(), '{');
        EXPECT_NE(jsonWithoutBracketsPretty.back(), '}');
    }
}
