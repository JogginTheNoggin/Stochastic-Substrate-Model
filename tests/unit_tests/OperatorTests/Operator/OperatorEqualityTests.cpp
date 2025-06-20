#include "gtest/gtest.h"
#include "helpers/TestOperator.h" // Changed to TestOperator
#include "headers/operators/Operator.h"
#include <memory>
#include <unordered_set> // Required for addConnectionInternal->getOutputConnections->DynamicArray<std::unordered_set>

// Fixture for Operator equality tests
class OperatorEqualityTest : public ::testing::Test {
protected:
    // Helper to create TestOperator
    std::unique_ptr<TestOperator> createOp(uint32_t id) {
        return std::make_unique<TestOperator>(id);
    }
};

// Test cases for Operator::equals, operator==, operator!=

TEST_F(OperatorEqualityTest, Equals_SameIdNoConnections) {
    std::unique_ptr<TestOperator> op1 = createOp(100); // Changed type
    std::unique_ptr<TestOperator> op2 = createOp(100); // Changed type

    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
    EXPECT_FALSE(*op1 != *op2);
}

TEST_F(OperatorEqualityTest, Equals_DifferentIdNoConnections) {
    std::unique_ptr<TestOperator> op1 = createOp(101); // Changed type
    std::unique_ptr<TestOperator> op2 = createOp(102); // Changed type

    EXPECT_FALSE(op1->equals(*op2)); // Operator::equals compares ID
    EXPECT_FALSE(*op1 == *op2);     // operator== will also be false due to ID
    EXPECT_TRUE(*op1 != *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdSameSingleConnection) {
    std::unique_ptr<TestOperator> op1 = createOp(103); // Changed type
    op1->addConnectionInternal(200, 1);
    std::unique_ptr<TestOperator> op2 = createOp(103); // Changed type
    op2->addConnectionInternal(200, 1);

    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdDifferentSingleConnectionTarget) {
    std::unique_ptr<TestOperator> op1 = createOp(104); // Changed type
    op1->addConnectionInternal(200, 1);
    std::unique_ptr<TestOperator> op2 = createOp(104); // Changed type
    op2->addConnectionInternal(201, 1); // Different target ID

    EXPECT_FALSE(op1->equals(*op2)); // Connection comparison should fail
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdDifferentSingleConnectionDistance) {
    std::unique_ptr<TestOperator> op1 = createOp(105); // Changed type
    op1->addConnectionInternal(200, 1);
    std::unique_ptr<TestOperator> op2 = createOp(105); // Changed type
    op2->addConnectionInternal(200, 2); // Different distance

    EXPECT_FALSE(op1->equals(*op2)); // Connection comparison should fail
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdMultipleConnectionsSameOrder) {
    std::unique_ptr<TestOperator> op1 = createOp(106); // Changed type
    op1->addConnectionInternal(200, 1);
    op1->addConnectionInternal(201, 1);
    op1->addConnectionInternal(202, 2);

    std::unique_ptr<TestOperator> op2 = createOp(106); // Changed type
    op2->addConnectionInternal(200, 1);
    op2->addConnectionInternal(201, 1);
    op2->addConnectionInternal(202, 2);

    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdMultipleConnectionsDifferentOrderInSet) {
    // Unordered_set means order of insertion for 200/201 doesn't guarantee read order,
    // but the sets themselves should be equal.
    std::unique_ptr<TestOperator> op1 = createOp(107); // Changed type
    op1->addConnectionInternal(200, 1); // Target 200
    op1->addConnectionInternal(201, 1); // Target 201

    std::unique_ptr<TestOperator> op2 = createOp(107); // Changed type
    op2->addConnectionInternal(201, 1); // Target 201 (added first to set)
    op2->addConnectionInternal(200, 1); // Target 200 (added second to set)

    // Operator::compareConnections compares unordered_sets, so order of elements within a set doesn't matter.
    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}


TEST_F(OperatorEqualityTest, Equals_SameIdOneMissingConnection) {
    std::unique_ptr<TestOperator> op1 = createOp(108); // Changed type
    op1->addConnectionInternal(200, 1);
    op1->addConnectionInternal(201, 2);

    std::unique_ptr<TestOperator> op2 = createOp(108); // Changed type
    op2->addConnectionInternal(200, 1); // op2 is missing connection to 201 at distance 2

    EXPECT_FALSE(op1->equals(*op2)); // compareConnections will find connB.count() different or a set mismatch
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdOneExtraConnection) {
    std::unique_ptr<TestOperator> op1 = createOp(109); // Changed type
    op1->addConnectionInternal(200, 1);

    std::unique_ptr<TestOperator> op2 = createOp(109); // Changed type
    op2->addConnectionInternal(200, 1);
    op2->addConnectionInternal(201, 2); // op2 has an extra connection

    EXPECT_FALSE(op1->equals(*op2));
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_IdenticalTestOperatorsViaCreateOp) { // Renamed and logic adjusted
    // TestOperator does not have additional derived parameters beyond Operator.
    // So, if base Operator parts are equal, TestOperators are equal.
    std::unique_ptr<TestOperator> op1 = createOp(110);
    op1->addConnectionInternal(300, 1);

    std::unique_ptr<TestOperator> op2 = createOp(110); // Same ID
    op2->addConnectionInternal(300, 1); // Same connection

    // TestOperator::equals calls Operator::equals. Since IDs and connections are the same,
    // Operator::equals will be true.
    EXPECT_TRUE(op1->equals(*op2)) << "TestOperator::equals should be true as base parts are identical.";

    // operator== first checks getOpType (both UNDEFINED for TestOperator via createOp).
    // Then it calls lhs.equals(rhs).
    EXPECT_TRUE(*op1 == *op2) << "operator== should be true for identical TestOperators.";
    EXPECT_FALSE(*op1 != *op2);
}

TEST_F(OperatorEqualityTest, Equals_IdenticalTestOperatorsWithConnections) { // Renamed and logic adjusted
    std::unique_ptr<TestOperator> op1 = std::make_unique<TestOperator>(111); // Direct creation
    op1->addConnectionInternal(300, 1);

    std::unique_ptr<TestOperator> op2 = std::make_unique<TestOperator>(111); // Direct creation, same ID
    op2->addConnectionInternal(300, 1); // Same connection

    // TestOperator::equals calls Operator::equals.
    EXPECT_TRUE(op1->equals(*op2)) << "TestOperator::equals should be true for identical setup.";
    EXPECT_TRUE(*op1 == *op2);
    EXPECT_FALSE(*op1 != *op2);
}

// Note: Testing operator== between different derived types (e.g., TestOperator vs a different future test operator)
// operator==(const Operator& lhs, const Operator& rhs) would return false
// if lhs.getOpType() != rhs.getOpType(). TestOperator always has type UNDEFINED.
