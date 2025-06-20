#include "gtest/gtest.h"
#include "helpers/TestOperator.h" // Changed to TestOperator
#include "headers/operators/Operator.h"
#include <memory>
#include "headers/util/DynamicArray.h"
#include <unordered_set>

class OperatorRemoveConnectionTest : public ::testing::Test {
};

TEST_F(OperatorRemoveConnectionTest, RemoveConnectionInternalBasic) {
    std::unique_ptr<TestOperator> op = std::make_unique<TestOperator>(10); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    // add two
    op->addConnectionInternal(100, 2);
    op->addConnectionInternal(101, 2);
    // remove one
    op->removeConnectionInternal(100, 2);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    const auto* bucket = connections.get(2);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(101), 1);
    EXPECT_EQ(bucket->count(100), 0);
}

TEST_F(OperatorRemoveConnectionTest, RemoveConnectionInternalLastFromBucket) {
    std::unique_ptr<TestOperator> op = std::make_unique<TestOperator>(11); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(110, 3);
    op->removeConnectionInternal(110, 3);

    const auto& connections = op->getOutputConnections();
    const auto* bucket = connections.get(3);
    ASSERT_EQ(bucket, nullptr); // empty bucket will be deleted
    // EXPECT_TRUE(bucket->empty());
}

TEST_F(OperatorRemoveConnectionTest, RemoveConnectionInternalNonExistentTarget) {
    std::unique_ptr<TestOperator> op = std::make_unique<TestOperator>(12); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(120, 1);
    op->removeConnectionInternal(121, 1);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    const auto* bucket = connections.get(1);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(120), 1);
}

TEST_F(OperatorRemoveConnectionTest, RemoveConnectionInternalNonExistentDistance) {
    std::unique_ptr<TestOperator> op = std::make_unique<TestOperator>(13); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(130, 4);
    op->removeConnectionInternal(130, 5);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    const auto* bucket = connections.get(4);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(130), 1);
}

TEST_F(OperatorRemoveConnectionTest, RemoveConnectionInternalNegativeDistance) {
    std::unique_ptr<TestOperator> op = std::make_unique<TestOperator>(14); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(140, 2);
    op->removeConnectionInternal(140, -1);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    const auto* bucket = connections.get(2);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(140), 1);
}

TEST_F(OperatorRemoveConnectionTest, RemoveConnectionInternalFromEmptyConnections) {
    std::unique_ptr<TestOperator> op = std::make_unique<TestOperator>(15); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->removeConnectionInternal(150, 0);

    const auto& connections = op->getOutputConnections();
    EXPECT_EQ(connections.count(), 0);
    EXPECT_EQ(connections.maxIdx(), -1);
}
