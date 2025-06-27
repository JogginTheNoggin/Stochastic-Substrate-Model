#include "gtest/gtest.h"
#include "helpers/MockOperator.h" // Changed to TestOperator
#include "headers/operators/Operator.h"
#include <memory>
#include "headers/util/DynamicArray.h"
#include <unordered_set>

class OperatorMoveConnectionTest : public ::testing::Test {
};

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalBasic) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(20); // Changed to TestOperator
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
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(21); // Changed to TestOperator
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
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(22); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(220, 1);
    op->moveConnectionInternal(221, 1, 4); // operator not existant, nothing is moved

    const auto& connections = op->getOutputConnections();
    const auto* bucketOld = connections.get(1);
    ASSERT_NE(bucketOld, nullptr);
    ASSERT_EQ(bucketOld->size(), 1);
    EXPECT_EQ(bucketOld->count(220), 1);

    const auto* bucketNew = connections.get(4);
    ASSERT_EQ(bucketNew, nullptr);
    //EXPECT_TRUE(bucketNew->empty());
    EXPECT_EQ(connections.count(), 1);
}


TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalFromNonExistentDistance) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(23); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(230, 2);
    op->moveConnectionInternal(230, 7, 4); // won't be created, op does not exist at 7, only 2

    const auto& connections = op->getOutputConnections();
    const auto* bucket2 = connections.get(2);
    ASSERT_NE(bucket2, nullptr);
    ASSERT_EQ(bucket2->size(), 1);
    EXPECT_EQ(bucket2->count(230), 1);

    const auto* bucket4 = connections.get(4);
    ASSERT_EQ(bucket4, nullptr);
    //ASSERT_EQ(bucket4->size(), 1);
    //EXPECT_EQ(bucket4->count(230), 1);

    EXPECT_EQ(connections.count(), 1);
}

TEST_F(OperatorMoveConnectionTest, MoveConnectionInternalToExistingBucket) {
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(24); // Changed to TestOperator
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
    std::unique_ptr<MockOperator> op = std::make_unique<MockOperator>(25); // Changed to TestOperator
    ASSERT_NE(op, nullptr);

    op->addConnectionInternal(250, 3);

    op->moveConnectionInternal(250, -1, 5); // will not create, returns without doing anything

    const auto& conns1 = op->getOutputConnections();
    const auto* b3_1 = conns1.get(3);
    ASSERT_NE(b3_1, nullptr); 
    EXPECT_EQ(b3_1->count(250), 1);
    const auto* b5_1 = conns1.get(5);
    ASSERT_EQ(b5_1, nullptr); 
    //EXPECT_EQ(b5_1->count(250), 1);
    EXPECT_EQ(conns1.count(), 1); // no new bucket has been made


    std::unique_ptr<MockOperator> op2 = std::make_unique<MockOperator>(26); // Changed to TestOperator
    op2->addConnectionInternal(260, 3); // add operator to bucket 3, should contain 2 now
    op2->moveConnectionInternal(260, 3, -1); // will not move to negative distance

    const auto& conns2 = op2->getOutputConnections();
    EXPECT_EQ(conns2.count(), 1); // new op (op2), 1 bucket and 1 connection
    const auto* b3_2 = conns2.get(3);
    ASSERT_NE(b3_2, nullptr);
    //EXPECT_TRUE(b3_2->empty());
    EXPECT_EQ(b3_2->size(), 1); // new op (op2) contains 1 connection at bucket 3
}
