#include "gtest/gtest.h"
#include "helpers/MockOperator.h"
#include "headers/operators/Operator.h"
#include <memory>
#include "headers/util/DynamicArray.h"
#include <unordered_set>

class OperatorAddConnectionTest : public ::testing::Test {
};

TEST_F(OperatorAddConnectionTest, AddConnectionInternalBasic) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(1);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(100, 2); // Target ID 100, distance 2

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    ASSERT_GE(connections.maxIdx(), 2);

    const auto* bucket = connections.get(2);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(100), 1);
}

TEST_F(OperatorAddConnectionTest, AddConnectionInternalMultipleAtSameDistance) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(2);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(200, 3);
    op->addConnectionInternal(201, 3);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    ASSERT_GE(connections.maxIdx(), 3);

    const auto* bucket = connections.get(3);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 2);
    EXPECT_EQ(bucket->count(200), 1);
    EXPECT_EQ(bucket->count(201), 1);
}

TEST_F(OperatorAddConnectionTest, AddConnectionInternalMultipleAtDifferentDistances) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(3);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(300, 1);
    op->addConnectionInternal(301, 5);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 2);
    ASSERT_GE(connections.maxIdx(), 5);

    const auto* bucket1 = connections.get(1);
    ASSERT_NE(bucket1, nullptr);
    ASSERT_EQ(bucket1->size(), 1);
    EXPECT_EQ(bucket1->count(300), 1);

    const auto* bucket5 = connections.get(5);
    ASSERT_NE(bucket5, nullptr);
    ASSERT_EQ(bucket5->size(), 1);
    EXPECT_EQ(bucket5->count(301), 1);
}

TEST_F(OperatorAddConnectionTest, AddConnectionInternalToExistingBucket) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(4);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(400, 2);
    op->addConnectionInternal(401, 2);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    ASSERT_GE(connections.maxIdx(), 2);

    const auto* bucket = connections.get(2);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 2);
    EXPECT_EQ(bucket->count(400), 1);
    EXPECT_EQ(bucket->count(401), 1);
}

TEST_F(OperatorAddConnectionTest, AddConnectionInternalNegativeDistance) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(5);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(500, -1);

    const auto& connections = op->getOutputConnections();
    EXPECT_EQ(connections.count(), 0);
    EXPECT_EQ(connections.maxIdx(), -1);
}

TEST_F(OperatorAddConnectionTest, AddConnectionInternalZeroDistance) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(6);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(600, 0);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    ASSERT_GE(connections.maxIdx(), 0);

    const auto* bucket = connections.get(0);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(600), 1);
}
