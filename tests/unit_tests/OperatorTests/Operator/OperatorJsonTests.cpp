#include "gtest/gtest.h"
#include "helpers/JsonTestHelpers.h"         // For readGoldenFile
#include "helpers/TestOperator.h" // Using TestOperator to test Operator's toJson
#include "headers/operators/Operator.h"
#include <memory>
#include <string>

// Test fixture for Operator toJson tests
class OperatorJsonTest : public ::testing::Test {};

namespace {
    std::string MOCK_FILE_DIR = "../tests/unit_tests/golden_files/operator/";

}
 


TEST_F(OperatorJsonTest, ToJsonNoConnections) {
    // ARRANGE: Use AddOperator as the concrete class to test the base Operator::toJson
    std::unique_ptr<Operator> op = std::make_unique<TestOperator>(123);
    std::string goldenOutput = JsonTestHelpers::readGoldenFile(MOCK_FILE_DIR + "operator_no_connections.json");

    // ACT: Call toJson with pretty-printing and enclosing brackets
    std::string actualOutput = op->toJson(true, true);

    // ASSERT: The output must exactly match the golden file
    EXPECT_EQ(actualOutput, goldenOutput);
}

TEST_F(OperatorJsonTest, ToJsonOneConnectionOneDistance) {
    // ARRANGE
    std::unique_ptr<Operator> op = std::make_unique<TestOperator>(124);
    op->addConnectionInternal(200, 2);
    std::string goldenOutput = JsonTestHelpers::readGoldenFile(MOCK_FILE_DIR + "operator_one_connection.json");

    // ACT
    std::string actualOutput = op->toJson(true, true);

    // ASSERT
    EXPECT_EQ(actualOutput, goldenOutput);
}

TEST_F(OperatorJsonTest, ToJsonMultipleConnectionsOneDistance) {
    // ARRANGE: Add connections out of order to test sorting of target IDs
    std::unique_ptr<Operator> op = std::make_unique<TestOperator>(125);
    op->addConnectionInternal(301, 1);
    op->addConnectionInternal(300, 1);
    std::string goldenOutput = JsonTestHelpers::readGoldenFile(MOCK_FILE_DIR + "operator_multi_conn_same_dist.json");

    // ACT
    std::string actualOutput = op->toJson(true, true);

    // ASSERT
    EXPECT_EQ(actualOutput, goldenOutput);
}

TEST_F(OperatorJsonTest, ToJsonMultipleConnectionsMultipleDistances) {
    // ARRANGE: Add distance buckets out of order to test sorting of buckets
    std::unique_ptr<Operator> op = std::make_unique<TestOperator>(126);
    op->addConnectionInternal(401, 3);
    op->addConnectionInternal(400, 0);
    op->addConnectionInternal(402, 3);
    std::string goldenOutput = JsonTestHelpers::readGoldenFile(MOCK_FILE_DIR + "operator_multi_conn_multi_dist.json");

    // ACT
    std::string actualOutput = op->toJson(true, true);

    // ASSERT
    EXPECT_EQ(actualOutput, goldenOutput);
}

TEST_F(OperatorJsonTest, ToJsonNoEnclosingBrackets) {
    // ARRANGE: For fragments, a hardcoded string is more practical than a golden file.
    std::unique_ptr<Operator> op = std::make_unique<TestOperator>(127);
    op->addConnectionInternal(500, 1);

    // Use a C++ raw string literal (R"()") for clean multi-line strings
    std::string expectedFragment = R"#(  "opType": "UNDEFINED",
  "operatorId": 127,
  "outputDistanceBuckets": [
    {
      "distance": 1,
      "targetOperatorIds": [
        500
      ]
    }
  ])#";

    // ACT
    std::string actualFragment = op->toJson(true, false);

    // ASSERT
    EXPECT_EQ(actualFragment, expectedFragment);
}