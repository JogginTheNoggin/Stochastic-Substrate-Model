#include "gtest/gtest.h"
#include "headers/operators/AddOperator.h"
#include "headers/operators/Operator.h"
#include <memory>
#include "headers/util/DynamicArray.h"
#include <unordered_set>

class OperatorMoveConnectionTest : public ::testing::Test {
};

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalBasic) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(20);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(200, 2);
    op->moveConnectionInternal(200, 2, 5);

    const auto& connections = op->getOutputConnections();
    const auto* bucketOld = connections.get(2);
    if (bucketOld != nullptr) {
        EXPECT_TRUE(bucketOld->empty());
    }

    const auto* bucketNew = connections.get(5);
    ASSERT_NE(bucketNew, nullptr);
    ASSERT_EQ(bucketNew->size(), 1);
    EXPECT_EQ(bucketNew->count(200), 1);
}

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalToSameDistance) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(21);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(210, 3);
    op->moveConnectionInternal(210, 3, 3);

    const auto& connections = op->getOutputConnections();
    ASSERT_EQ(connections.count(), 1);
    const auto* bucket = connections.get(3);
    ASSERT_NE(bucket, nullptr);
    ASSERT_EQ(bucket->size(), 1);
    EXPECT_EQ(bucket->count(210), 1);
}

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalNonExistentTarget) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(22);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(220, 1);
    op->moveConnectionInternal(221, 1, 4);

    const auto& connections = op->getOutputConnections();
    const auto* bucketOld = connections.get(1);
    ASSERT_NE(bucketOld, nullptr);
    ASSERT_EQ(bucketOld->size(), 1);
    EXPECT_EQ(bucketOld->count(220), 1);

    const auto* bucketNew = connections.get(4);
    ASSERT_NE(bucketNew, nullptr);
    EXPECT_TRUE(bucketNew->empty());
}


TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalFromNonExistentDistance) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(23);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(230, 2);
    op->moveConnectionInternal(230, 7, 4);

    const auto& connections = op->getOutputConnections();
    const auto* bucket2 = connections.get(2);
    ASSERT_NE(bucket2, nullptr);
    ASSERT_EQ(bucket2->size(), 1);
    EXPECT_EQ(bucket2->count(230), 1);

    const auto* bucket4 = connections.get(4);
    ASSERT_NE(bucket4, nullptr);
    ASSERT_EQ(bucket4->size(), 1);
    EXPECT_EQ(bucket4->count(230), 1);

    EXPECT_EQ(connections.count(), 2);
}

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalToExistingBucket) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(24);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(240, 1);
    op->addConnectionInternal(241, 6);

    op->moveConnectionInternal(240, 1, 6);

    const auto& connections = op->getOutputConnections();
    const auto* bucketOld = connections.get(1);
    if (bucketOld != nullptr) {
        EXPECT_TRUE(bucketOld->empty());
    }

    const auto* bucketNew = connections.get(6);
    ASSERT_NE(bucketNew, nullptr);
    ASSERT_EQ(bucketNew->size(), 2);
    EXPECT_EQ(bucketNew->count(240), 1);
    EXPECT_EQ(bucketNew->count(241), 1);
}

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalWithNegativeDistances) {
    std::unique_ptr<AddOperator> op = std::make_unique<AddOperator>(25);
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(250, 3);

    op->moveConnectionInternal(250, -1, 5);

    const auto& conns1 = op->getOutputConnections();
    const auto* b3_1 = conns1.get(3);
    ASSERT_NE(b3_1, nullptr); EXPECT_EQ(b3_1->count(250), 1);
    const auto* b5_1 = conns1.get(5);
    ASSERT_NE(b5_1, nullptr); EXPECT_EQ(b5_1->count(250), 1);
    EXPECT_EQ(conns1.count(), 2);


    std::unique_ptr<AddOperator> op2 = std::make_unique<AddOperator>(26);
    op2->addConnectionInternal(260, 3);
    op2->moveConnectionInternal(260, 3, -1);

    const auto& conns2 = op2->getOutputConnections();
    const auto* b3_2 = conns2.get(3);
    ASSERT_NE(b3_2, nullptr);
    EXPECT_TRUE(b3_2->empty());
}
