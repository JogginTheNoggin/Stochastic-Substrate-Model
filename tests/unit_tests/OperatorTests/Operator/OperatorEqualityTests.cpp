#include "gtest/gtest.h"
#include "headers/operators/AddOperator.h" // Using AddOperator for tests
#include "headers/operators/Operator.h"
#include <memory>
#include <unordered_set> // Required for addConnectionInternal->getOutputConnections->DynamicArray<std::unordered_set>

// Fixture for Operator equality tests
class OperatorEqualityTest : public ::testing::Test {
protected:
    // Helper to create AddOperator with specific parameters for testing
    // AddOperator(int id, int initialWeight = 1, int initialThreshold = 0);
    std::unique_ptr<AddOperator> createOp(uint32_t id, int weight = 1, int threshold = 0) {
        // AddOperator's constructor takes int for id.
        return std::make_unique<AddOperator>(static_cast<int>(id), weight, threshold);
    }
};

// Test cases for Operator::equals, operator==, operator!=

TEST_F(OperatorEqualityTest, Equals_SameIdNoConnections) {
    auto op1 = createOp(100);
    auto op2 = createOp(100);

    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
    EXPECT_FALSE(*op1 != *op2);
}

TEST_F(OperatorEqualityTest, Equals_DifferentIdNoConnections) {
    auto op1 = createOp(101);
    auto op2 = createOp(102);

    EXPECT_FALSE(op1->equals(*op2)); // Operator::equals compares ID
    EXPECT_FALSE(*op1 == *op2);     // operator== will also be false due to ID
    EXPECT_TRUE(*op1 != *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdSameSingleConnection) {
    auto op1 = createOp(103);
    op1->addConnectionInternal(200, 1);
    auto op2 = createOp(103);
    op2->addConnectionInternal(200, 1);

    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdDifferentSingleConnectionTarget) {
    auto op1 = createOp(104);
    op1->addConnectionInternal(200, 1);
    auto op2 = createOp(104);
    op2->addConnectionInternal(201, 1); // Different target ID

    EXPECT_FALSE(op1->equals(*op2)); // Connection comparison should fail
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdDifferentSingleConnectionDistance) {
    auto op1 = createOp(105);
    op1->addConnectionInternal(200, 1);
    auto op2 = createOp(105);
    op2->addConnectionInternal(200, 2); // Different distance

    EXPECT_FALSE(op1->equals(*op2)); // Connection comparison should fail
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdMultipleConnectionsSameOrder) {
    auto op1 = createOp(106);
    op1->addConnectionInternal(200, 1);
    op1->addConnectionInternal(201, 1);
    op1->addConnectionInternal(202, 2);

    auto op2 = createOp(106);
    op2->addConnectionInternal(200, 1);
    op2->addConnectionInternal(201, 1);
    op2->addConnectionInternal(202, 2);

    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdMultipleConnectionsDifferentOrderInSet) {
    // Unordered_set means order of insertion for 200/201 doesn't guarantee read order,
    // but the sets themselves should be equal.
    auto op1 = createOp(107);
    op1->addConnectionInternal(200, 1); // Target 200
    op1->addConnectionInternal(201, 1); // Target 201

    auto op2 = createOp(107);
    op2->addConnectionInternal(201, 1); // Target 201 (added first to set)
    op2->addConnectionInternal(200, 1); // Target 200 (added second to set)

    // Operator::compareConnections compares unordered_sets, so order of elements within a set doesn't matter.
    EXPECT_TRUE(op1->equals(*op2));
    EXPECT_TRUE(*op1 == *op2);
}


TEST_F(OperatorEqualityTest, Equals_SameIdOneMissingConnection) {
    auto op1 = createOp(108);
    op1->addConnectionInternal(200, 1);
    op1->addConnectionInternal(201, 2);

    auto op2 = createOp(108);
    op2->addConnectionInternal(200, 1); // op2 is missing connection to 201 at distance 2

    EXPECT_FALSE(op1->equals(*op2)); // compareConnections will find connB.count() different or a set mismatch
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameIdOneExtraConnection) {
    auto op1 = createOp(109);
    op1->addConnectionInternal(200, 1);

    auto op2 = createOp(109);
    op2->addConnectionInternal(200, 1);
    op2->addConnectionInternal(201, 2); // op2 has an extra connection

    EXPECT_FALSE(op1->equals(*op2));
    EXPECT_FALSE(*op1 == *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameBaseDifferentDerivedParams) {
    // op1 and op2 will have same ID and connections (Operator part is equal)
    // But different AddOperator params (weight/threshold)
    auto op1 = createOp(110, 10, 20); // weight 10, threshold 20
    op1->addConnectionInternal(300, 1);

    auto op2 = createOp(110, 15, 25); // weight 15, threshold 25
    op2->addConnectionInternal(300, 1);

    // Operator::equals(*op2) would be true if called directly on base pointers of same type.
    // However, we are calling AddOperator::equals which calls Operator::equals
    // and then compares its own members.
    // So, op1->equals(*op2) should be false because AddOperator members differ.
    EXPECT_FALSE(op1->equals(*op2)) << "AddOperator::equals should be false due to different weight/threshold";

    // operator== first checks getOpType (which is same: ADD for both).
    // Then it calls lhs.equals(rhs), which is op1->equals(*op2) -> AddOperator::equals
    EXPECT_FALSE(*op1 == *op2) << "operator== should be false due to different AddOperator params";
    EXPECT_TRUE(*op1 != *op2);
}

TEST_F(OperatorEqualityTest, Equals_SameBaseAndDerivedParams) {
    auto op1 = createOp(111, 10, 20);
    op1->addConnectionInternal(300, 1);

    auto op2 = createOp(111, 10, 20); // Same weight and threshold
    op2->addConnectionInternal(300, 1);

    EXPECT_TRUE(op1->equals(*op2)); // AddOperator::equals should be true
    EXPECT_TRUE(*op1 == *op2);
    EXPECT_FALSE(*op1 != *op2);
}

// Note: Testing operator== between different derived types (e.g., AddOperator vs HypoSubOperator)
// is not possible here as we only have AddOperator. If other types existed,
// operator==(const Operator& lhs, const Operator& rhs) would return false
// if lhs.getOpType() != rhs.getOpType().
